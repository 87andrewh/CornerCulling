# CornerCulling
Fast and accurate culling method. Proof of concept in C++ and UE4.
Designed to prevent wallhacks in FPS games.

Given 50 LOS checks and 360 possible occluding objects,
computes maximally precise occlusion in 0.02 ms (worst case) on i5 6600k.
Going up to 1500 objects, computes occlusion in 0.03 ms (average case).

Unbeknownst to me until now (but unsurprisingly), my idea is essential the same as
Shadow Culling, which graphics researchers documented a year before I was born.
https://www.gamasutra.com/view/feature/3394/occlusion_culling_algorithms.php?print=1
I suspect that I could find further improvements made in the past 20 years.

Tasks (in no order):
1)  Implement potentialy visible sets to cull enemies.
    Also consider culling boxes themselves.
2)  Test performance of occluding surfaces, aka 2D walls instead of boxes
3)  Consider better ways to incorperate Z axis information.
4)  Reach out to more FPS game developers.
5)  Continue researching graphics community state of the art.
6)  What to do about a wallhacking Jet with a lag switch?
7)  Update demos.

Demo:
https://youtu.be/e6rjJtcdKfw
