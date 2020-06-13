# CornerCulling
Fast and maximally accurate culling method. Proof of concept in C++ and UE4.
Designed to prevent wallhacks in FPS games. Wallhack penicillin.

Runtime demonstration:
https://youtu.be/SHUXDR0hleU

Accuracy demonstration:
https://youtu.be/tzrIXcdYQJE

Critically, also accounts for client latency, allowing the server to cull according to where a laggy player could be in the future.

Given 50 LOS checks and 360 possible occluding objects,
computes maximally precise occlusion in 0.02 ms (worst case) on i5 6600k.
Going up to 1500 objects, computes occlusion in 0.04 ms (average case) and 0.01 ms (worst case).
Occlusion is maximal if it reveals as little as possible while still preventing
enemies from "popping" into existence when a laggy player peeks.

Major Task:
Big refactor ¯\_(ツ)_/¯
Move occlusion logic to OcclusionController class.
Disentangle VisibilityPrisms from players and occluding objects.

Refactor design doc:
    Culling controller handles all culling. New culling loop.
    For every team:
        For every player:
            For enemy in Enemies(player):
                If PotentiallyVisible(player, enemy) and not LingeringVisibility(player, enemy):
                    Add enemy to EnemyQueue
            For enemy in EnemyQueue:
                For every segment in SegmentCache(player):
                    LOS check   // should be common case to short circuit here
            For object in OccludingObjects:
                If PotentiallyVisible(player, object):
                    Add RelevantSegment(player, object) to SegmentQueue
            For every enemy in EnemyQueue:
                For every segment in SegmentQueue:
                    LOS check
                    Update SegmentCache by LRU
                    
Imagine hitting 1 ms while culling for Fortnite.
               
Other Tasks (in no order):
1)  Implement potentialy visible sets to cull enemies and occluding objects.
2)  Test performance of occluding surfaces, aka 2D walls instead of boxes
3)  Calculate Z visibility by projecting from top of player to top of wall in the direction
    of the enemy. If this angle hits below the top of the enemy, or hits the smicircle bounding the top
    of the enemy, reveal the enemy.
4)  Reach out to more FPS game developers.
5)  Continue researching graphics community state of the art.
6)  What to do about a wallhacking Jet with a lag switch?
7)  Update demos.
8)  Make enemy lingering visibility apaptive only when server is under load.
9)  Use existence-based predication for caches. Test second chance and random replacement algorithms.
10) Change "RelevantCorners" to just "LeftAndRightCorners" for readability.
11) Design doc opimizations for large Battle Royale type games.
    No culling until enough players die. PVS filter players and occluders.


Unbeknownst to me until now (but unsurprisingly), my idea is essential the same as
Shadow Culling, which graphics researchers documented in 1997. <br />
https://www.gamasutra.com/view/feature/3394/occlusion_culling_algorithms.php?print=1 <br />
I suspect that I could incorperate improvements made in the past 20 years.
