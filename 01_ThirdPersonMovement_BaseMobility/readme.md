# 01 Third Person Movement : Base Mobility

Welcome to the Base Mobility section of the LVN Gameplay Programming Showcase series.

This section demonstrates a fully functional third-person movement system implemented in both **Unity (C#)** and **Unreal Engine (C++ & Blueprint screenshots)**.

<h2 align="center">Overview</h2>

<p align="center">
  <img src="https://github.com/user-attachments/assets/a636ccfc-e654-406a-90d1-e23151ee5758" width="600px" />
</p>

This section focuses on a **3D third-person movement system** with the following features:

- Basic walking in all directions
- Sprinting while moving forward or sideways (never backward)
- Walking backwards with proper rotation animation logic
- Free orbiting camera with adjustable sensitivity and pitch clamping
- Smooth rotation coordinated with camera direction
- Fully animated character from Mixamo (animations handled externally on the unreal script, code is independent of animation logic)
- Dance ability can be triggered when idle (ends automatically when movement starts)
- Customizable parameters: walk speed, sprint speed, camera pitch and sensitivity, step offset, slope angle...
- Works for both **keyboard & mouse** and **gamepad controllers**, if properly configured

> Note: This repository **does not include engine setup instructions**. To run these scripts, you may need to connect input actions, assign components, or configure settings as appropriate in your project.
