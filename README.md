# CornerCulling
Fast and maximally accurate culling method. Proof of concept in C++ and UE4.
Designed to prevent wallhacks in FPS games. Wallhack penicillin.

Speed demo
https://youtu.be/SHUXDR0hleU

Given 50 LOS checks and 360 possible occluding objects,
computes maximally precise occlusion in 0.02 ms (worst case) on i5 6600k.
Going up to 1500 objects, computes occlusion in 0.04 ms (average case) and 0.01 ms (worst case).
Occlusion is maximal if it reveals as little as possible while still preventing
enemies from "popping" into existence when a laggy player peeks.

Unbeknownst to me until now (but unsurprisingly), my idea is essential the same as
Shadow Culling, which graphics researchers documented a year before I was born. <br />
https://www.gamasutra.com/view/feature/3394/occlusion_culling_algorithms.php?print=1 <br />
I suspect that I could find further improvements made in the past 20 years.
Also, I think I found the orignal PVS paper. It's a 200 page disseration. <br />
https://www2.eecs.berkeley.edu/Pubs/TechRpts/1992/CSD-92-708.pdf <br />
For my project, I will probably write a 200 sentence blog post.

Tasks (in no order):
1)  Implement potentialy visible sets to cull enemies and occluding objects.
2)  Test performance of occluding surfaces, aka 2D walls instead of boxes
3)  Consider better ways to incorperate Z axis information.
4)  Reach out to more FPS game developers.
5)  Continue researching graphics community state of the art.
6)  What to do about a wallhacking Jet with a lag switch?
7)  Update demos.
8)  Make enemy lingering visibility apaptive only when server is under load.
