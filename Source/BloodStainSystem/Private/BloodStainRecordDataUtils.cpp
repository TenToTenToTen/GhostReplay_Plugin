/*
* Copyright 2025 TenToTen, All Rights Reserved.
*/


#include "BloodStainRecordDataUtils.h"
#include "BloodStainSystem.h"
#include "GhostData.h"

namespace BloodStainRecordDataUtils
{
	bool CookQueuedFrames(float SamplingInterval, const float& ClipStartTime, TCircularQueue<FRecordFrame>* FrameQueuePtr, FRecordActorSaveData& OutGhostSaveData, TArray<FComponentActiveInterval>& OutComponentIntervals)
	{
		FRecordFrame First;
		if (!FrameQueuePtr->Peek(First))
		{
			UE_LOG(LogBloodStain, Warning, TEXT("No frames to save"));
			return false;
		}

		int32 FirstIndex = -1;

		// Copy original frame datas and do normalize timestamps [0, duration)
		TArray<FRecordFrame> RawFrames;
		FRecordFrame Tmp;
		while (FrameQueuePtr->Dequeue(Tmp))
		{
			Tmp.TimeStamp -= ClipStartTime;
			if (Tmp.TimeStamp < 0)
			{
				continue;
			}

			if (RawFrames.Num() == 0)
			{
				FirstIndex = Tmp.FrameIndex;
			}
			
			RawFrames.Add(MoveTemp(Tmp));
		}
		
		if (RawFrames.Num() < 2)
		{
			UE_LOG(LogBloodStain, Warning, TEXT("Not enough raw frames to interpolate."));
			return false;
		}
		
		OutGhostSaveData.RecordedFrames = RawFrames;
		
		/* Construct Initial Component Structure based on Total Component event Data */
		BuildInitialComponentStructure(FirstIndex, OutGhostSaveData, OutComponentIntervals);
	
		return true;
	}

	void BuildInitialComponentStructure(int32 FirstFrameIndex, FRecordActorSaveData& OutGhostSaveData, TArray<FComponentActiveInterval>& OutComponentIntervals)
	{
		int32 NumSavedFrames = OutGhostSaveData.RecordedFrames.Num();
		OutComponentIntervals.Sort([](auto& A, auto& B) {
			return A.EndFrame < B.EndFrame;
		});

		// StartIdx : first index where EndFrame > FirstFrameIndex 
		const int32 StartIdx = Algo::UpperBoundBy(OutComponentIntervals, FirstFrameIndex, &FComponentActiveInterval::EndFrame);

		for (int32 i = StartIdx; i < OutComponentIntervals.Num(); ++i)
		{
			FComponentActiveInterval& Interval = OutComponentIntervals[i];
			
			Interval.StartFrame = FMath::Max(0, Interval.StartFrame - FirstFrameIndex);
			
			if (Interval.EndFrame == INT32_MAX)
			{
				Interval.EndFrame = NumSavedFrames;
			}
			else
			{
				Interval.EndFrame = FMath::Min(Interval.EndFrame - FirstFrameIndex, NumSavedFrames);
			}

			OutGhostSaveData.ComponentIntervals.Add(Interval);
			UE_LOG(LogBloodStain, Log, TEXT("BuildInitialComponentStructure: %s added to initial structure"),
				   *Interval.Meta.ComponentName);
		}
	}

	void ClipActorSaveDataByGroup(TArray<FRecordActorSaveData>& Actors, float MaxGroupRecordTime, float SamplingInterval)
	{
		if (Actors.Num() == 0)
		{
			UE_LOG(LogBloodStain, Warning, TEXT("ClipActorSaveDataByGroup: No actors to process."));
			return;
		}

		float GroupStartTime = FLT_MAX;
		float GroupEndTime   = -FLT_MAX;

		for (const FRecordActorSaveData& D : Actors)
		{
			GroupStartTime = FMath::Min(GroupStartTime, D.RecordedFrames[0].TimeStamp);
			GroupEndTime   = FMath::Max(GroupEndTime,   D.RecordedFrames.Last().TimeStamp);
		}

		const float& WindowEnd = GroupEndTime;
		float WindowStart = FMath::Max(GroupStartTime, WindowEnd - MaxGroupRecordTime);

		
		for (FRecordActorSaveData& Actor : Actors)
		{
			if (Actor.RecordedFrames.Last().TimeStamp < WindowStart || Actor.RecordedFrames[0].TimeStamp > WindowEnd)
			{
				Actor.RecordedFrames.Empty();
				Actor.ComponentIntervals.Empty();
				
				continue;
			}

			auto& Frames = Actor.RecordedFrames;
			const int32 Length = Frames.Num();
			
			float LocalStart = WindowStart - Actor.RecordedFrames[0].TimeStamp;
			float LocalEnd   = WindowEnd   - Actor.RecordedFrames[0].TimeStamp;
			
			int32 StartIdx = Algo::LowerBoundBy(Frames, LocalStart, [](const FRecordFrame& Frame) {;
				return Frame.TimeStamp;
			});

			int32 EndIdx = Algo::UpperBoundBy(Frames, LocalEnd, [](const FRecordFrame& Frame) {
				return Frame.TimeStamp;
			}) - 1;

			StartIdx = FMath::Clamp(StartIdx, 0, Length - 1);
			EndIdx = FMath::Clamp(EndIdx, 0, Length - 1);
			
			if (StartIdx > EndIdx)
			{
				Frames.Empty();
				Actor.ComponentIntervals.Empty();
				continue;
			}

			float NewFirstWorldTime = Actor.RecordedFrames[0].TimeStamp + Frames[StartIdx].TimeStamp;

			TArray<int32> OldToNew;
			OldToNew.Init(-1, Length);
			int32 NewCount = EndIdx - StartIdx + 1;
			for (int32 i = StartIdx; i <= EndIdx; ++i)
			{
				OldToNew[i] = i - StartIdx;
			}
			
			TArray<FRecordFrame> NewFrames;
			NewFrames.Reserve(EndIdx - StartIdx + 1);
			for (int32 i = StartIdx; i <= EndIdx; ++i)
			{
				const FRecordFrame& O = Frames[i];
				FRecordFrame G = O;

				float WorldTime = Actor.RecordedFrames[0].TimeStamp + O.TimeStamp;
				G.TimeStamp = WorldTime - NewFirstWorldTime;

				NewFrames.Add(MoveTemp(G));
			}
			Actor.RecordedFrames = MoveTemp(NewFrames);

			Actor.RecordedFrames[0].TimeStamp = NewFirstWorldTime;
			if (!Actor.RecordedFrames.IsEmpty())
			{
				Actor.RecordedFrames.Last().TimeStamp = NewFirstWorldTime + Actor.RecordedFrames.Last().TimeStamp;
			}
			else
			{
				Actor.RecordedFrames.Last().TimeStamp = NewFirstWorldTime;
			}

			TArray<FComponentActiveInterval> NewIntervals;
			NewIntervals.Reserve(Actor.ComponentIntervals.Num());

			for (auto const& I : Actor.ComponentIntervals)
			{
				// 원본 [I.StartFrame, I.EndFrame)
				int32 s = INDEX_NONE, e = INDEX_NONE;

				// newStart 찾기
				for (int32 o = I.StartFrame; o < I.EndFrame && o < Length; ++o)
				{
					if (OldToNew[o] != -1)
					{
						s = OldToNew[o];
						break;
					}
				}
				if (s == INDEX_NONE) 
					continue;

				// newEnd 찾기 (inclusive → exclusive)
				for (int32 o = FMath::Min(I.EndFrame - 1, Length - 1); o >= I.StartFrame; --o)
				{
					if (OldToNew[o] != -1)
					{
						e = OldToNew[o] + 1;
						break;
					}
				}
				if (e <= s) 
					continue;

				FComponentActiveInterval NI = I;
				NI.StartFrame = FMath::Clamp(s, 0, NewCount);
				NI.EndFrame   = FMath::Clamp(e, 0, NewCount);
				NewIntervals.Add(MoveTemp(NI));
			}

			Actor.ComponentIntervals = MoveTemp(NewIntervals);
			
		}
	}
}
