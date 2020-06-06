# CornerCulling
Fast and accurate culling method. Proof of concept in C++ and UE4.
Designed to prevent wallhacks in FPS games.

Given 50 LOS checks and 40 possible occluding objects,
computes pixel-perfect occlusion in ~100 microseconds on i5 6600k.

Demo:
https://youtu.be/e6rjJtcdKfw
