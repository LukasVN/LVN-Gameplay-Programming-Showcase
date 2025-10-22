# 02 Third Person Movement : Jumping

The jumping system builds directly on the [**01 Base Mobility**](https://github.com/LukasVN/LVN-Gameplay-Programming-Showcase/tree/main/01_ThirdPersonMovement_BaseMobility) foundation, extending its movement logic with vertical traversal and mid-air control.

This feature is **animation-based**, meaning the actual jump force is triggered by a keyframe event embedded in the jump and flip (double jump) animations. While this approach may not always feel perfectly responsive, it ensures that animations play in tight coordination with gameplay behavior, maintaining visual consistency and timing.


### Key Features:
- Jump buffering for responsive input
- Flip (double jump) available mid-air if enabled
- Instead of adding Coyote time the flip can be triggered if falling without previous jumping
- Jump and flip force triggered via animation notifies (Unreal) or animation events (Unity)
- Clean state transitions between jump, fall, flip, and land
- Velocity reset before flip to ensure consistent launch height
- Fully synchronized with Mixamo animations
- Customizable parameters including (among others) jump force, flip force, buffer timing, gravity scale (Unreal), air control (Unreal), ground detection (Unity), and double jump toggle

> Note: The jump system is implemented in both **Unity (C#)** and **Unreal Engine (Blueprint)**, with Unreal using animation notifies to trigger physics actions and Unity using animation events.
