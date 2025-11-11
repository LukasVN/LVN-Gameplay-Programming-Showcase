# 04 Third Person Movement : Sliding

Welcome to the Sliding section of the LVN Gameplay Programming Showcase series.

Building on the [**03 Crouching and Proning Module**](https://github.com/LukkasVN/LVN-Gameplay-Programming-Showcase/tree/main/03_ThirdPersonMovement_Crouching_%26_Proning), this entry introduces a friction-based sliding mechanic that reacts dynamically to player input, movement speed, and terrain slope.

<h2 align="center">Overview</h2>

<p align="center">
  <img src="https://github.com/user-attachments/assets/fbb9f341-b419-400e-b1aa-343bf04852e6" width="600px" />
</p>

The sliding system activates when the player is holding the run button and presses the crouch key (which doubles as the slide input).  
Once triggered, the character continues sliding until friction or an upward slope slows them down, or until the player releases the slide key.  
On downward slopes, slides naturally last longer, while uphill surfaces shorten the duration.

The capsule height and mesh alignment adapt dynamically to match the animation, allowing the player to slide through tight spaces.  
If the slide ends inside a low-clearance area, the system automatically transitions into the appropriate stance (crouch or prone), ensuring seamless interaction with the environment and consistent character fitting.

<h3>Key features include:</h3>

- Slide triggered by holding run and pressing crouch or slide key  
- Friction-based sliding that adapts to terrain slope and momentum  
- Automatic transition to crouch or prone if the slide ends in low clearance  
- Capsule resizing and mesh alignment synced to animation
- Allows the player to start sliding mid-air if close to the ground
- Smooth state blending between stand, crouch, prone, and slide   
- Consistent performance on flat, uphill, and downhill terrain
- Animated using Free Mixamo animations

> Note: Unreal uses `SetCapsuleHalfHeight(..., true)` for smooth capsule adaptation during slides and slope transitions. Unity uses `CharacterController.height` with friction and velocity adjustments to simulate slide momentum.

> Extra Note: The Sliding system is implemented in both **Unity (C#)** and **Unreal Engine (Blueprint)**, keeping visual synchronization and gameplay feel consistent across both engines.
