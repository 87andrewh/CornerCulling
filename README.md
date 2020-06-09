# CornerCulling
Fast and accurate culling method. Proof of concept in C++ and UE4.
Designed to prevent wallhacks in FPS games.

Given 50 LOS checks and 360 possible occluding objects,
computes pixel-perfect occlusion in 0.25 milliseconds on i5 6600k.


Tasks (in no order):
1) Implement potentialy visible sets to cull enemies. Also consider culling boxes themselves.
2) Test performance of occluding surfaces, aka 2D walls instead of boxes
3) Consider better ways to incorperate Z axis information.
4) Reach out to more FPS game developers.
5) Continue researching graphics community state of the art.
6) What to do about a wallhacking Jet with a lag switch?
7) Update demos.
8) Get rid of code duplication in AngleCull and LineCull. Or just delete one.

Demo:
https://youtu.be/e6rjJtcdKfw
