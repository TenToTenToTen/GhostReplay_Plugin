## 🌀 Ghost Replay System

Bring your world to life with **instant replays** — whether it's watching how a player died in a Soulslike bloodstain, or racing against your past self with a ghost car.

**Ghost Replay Plugin** makes it easy to record and replay any actor's animation, pose, and visual state — in both singleplayer and multiplayer. No complex setup required.
    
### ✨ Key Features

- 🎥 **Instant Replay for Any Actor**
    - Record and replay actors with **Static or Skeletal Meshes**
    - Captures full pose (including IK, bone copy, etc.) and component transforms
    - Supports advanced simulation components like **Groom Hair** and **Chaos Cloth**
- 🎮 **Multiplayer-Ready by Design**
    - Built-in support for **dedicated and listen servers**
    - Smart **data chunking** system avoids reliable buffer overflow
    - Lightweight replay files via **quantization and compression**, ideal for online play
- 👥 **Record Multiple Actors at Once**
    - Grouped or simultaneous multi-actor recording and playback
    - Each actor’s pose, animation, and materials are restored accurately
- 🧱 **Drop-In Integration**
    - Works seamlessly with existing projects like **Lyra**, **Valley of the Ancient**, and more
    - Add bloodstains, replay triggers, and ghost actors with minimal setup
- 🎨 **Visual Fidelity**
    - Materials are dynamically re-instanced and restored per actor
    - Custom playback options: loop, reverse, play rate, visibility control

### 🕹 Use Cases

<img src="{{ site.baseurl }}/images/Plugin_Overview/01_Demo2_Bloodstain.jpg" alt="BloodStain Demo Image" width="500"/>
- **🩸 Bloodstain Replays**
    
    > Let players touch a bloodstain and witness how another player died — just like Soulslike games.
    > 

<img src="{{ site.baseurl }}/images/Plugin_Overview/01_Demo1_Ghostcar.jpg" alt="Ghost Car Demo Image" width="500"/>
- **🏁 Ghost Car Challenges**
    
    > Replay your best lap and challenge your past self or another player’s ghost in real time.
    > 
- **📚 In-World Tutorials or Story Moments**
    
    > Embed recorded scenes or actions into the game world as ambient narrative or guidance tools.
    > 

### 🔍 What Sets It Apart

- ✅ **Multiplayer-aware from the ground up**
    
    Efficiently synchronizes replay data without flooding the network.
    
- ✅ **Fully modular**
    
    No need to modify your existing animation systems or blueprints — plug and play.
    
- ✅ **Replay accuracy with performance in mind**
    
    Uses **FAnimInstanceProxy** for threaded animation evaluation and **quantized transform storage** for minimal bandwidth impact.
    
- ✅ **Visual separation with orchestration**
    
    Server and client replay actors are clearly separated between data orchestrators and visual ghosts — reducing replication and improving clarity.
