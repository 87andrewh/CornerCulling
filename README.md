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
Big refactor ¯\\\_(ツ)_/¯  
Move occlusion logic to OcclusionController class.  
Disentangle VisibilityPrisms from players and occluding objects.  

Refactor design doc:  
    Culling controller handles all culling. New culling loop.  
    
```python
# Get line segments needed to calculate LOS between each player and their enemies.
for player_i in player_indicies:  
    for enemy_i in player_indicies:
        if (get_team(player_i) != get_team(enemy_i) and
            (almost_visible(player_i, enemy_i) or
             ((cull_this_tick() and
              potentially_visible(player_i, enemy_i)
             )
            )
           ):  
            LOS_segments_queue.push(get_LOS_segments(player_i, enemy_i))

# Try to block segments with occluding planes in each segments' player's cache.
# This case should be common and fast.
for segments in LOS_segments_queue:
    for plane in occluding_plane_caches[segments.player_i]:  
        if (intersects(segments.left, plane) or
            intersects(segments.right, plane)
           ):
            blocked_queue.push(segment)
            break
    LOS_segment_queue_2.push(segment)

# Get occluding planes for all potentialy visible occluders.
for player_i in player_indicies:  
    for occluder in occluding_prisms:  
        if potentially_visible(player_i, occluder):  
            occluding_plane_queue.push(get_occluding_finite_plane(player_i, occluder))

# Check remaining LOS segments against all occluding planes in the queue.
for segments in LOS_segments_queue_2:
    for plane in occluding_plane_queue:  
        if (intersects(segment.left, plane) or
            intersects(segment.right, plane)
           ):
            blocked_queue.push(segment)
            update_cache_LRU(occluding_plane_caches[segments.player_i], plane)
            break

for segments in blocked_queue:
    hide(segments.player_i, segments.enemy_i)
reset_queues()
```

                    
Imagine hitting 1 ms while culling for Fortnite. Might have to wait for some people to die.
               
Other Tasks (in no order):
1)  Implement (or hack together) potentialy visible sets to cull enemies and occluding objects.
2)  Test performance of occluding surfaces, aka 2D walls instead of boxes
3)  Calculate Z visibility by projecting from top of player to top of wall in the direction
    of the enemy. If this angle hits below the top of the enemy, or hits the smicircle bounding the top
    of the enemy, reveal the enemy.
4)  Reach out to more FPS game developers.
5)  Continue researching graphics community state of the art.
6)  What to do about a wallhacking Jet with a lag switch?
7)  Update demos.
8)  Make enemy lingering visibility adaptive only when server is under load.
9)  Use existence-based predication for caches. Test LRU, k-th chance, and random replacement algorithms.
10) Change "RelevantCorners" to just "LeftAndRightCorners" for readability.
11) Design doc opimizations for large Battle Royale type games.
    No culling until enough players die. PVS filter players and occluders.

Unbeknownst to me until now (but unsurprisingly), my idea is essential the same as
Shadow Culling, which graphics researchers documented in 1997. <br />
https://www.gamasutra.com/view/feature/3394/occlusion_culling_algorithms.php?print=1 <br />
I suspect that I could incorperate improvements made in the past 20 years.
