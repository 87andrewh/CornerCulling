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
for team in teams:  
    for player_i in team:  
        for enemy_i in enemies(player_i):  
            if (almost_visible(player_i, enemy_i) or
                (cull_this_tick() and
                 potentially_visible(player_i, enemy_i)
                )
               ):  
                enemy_qeueu.push(enemy_i) 
        for enemy_i in enemy_queue:  
            left_LOS_segment, right_LOS_segment = get_LOS_segments(player_i, enemy_i)
            for plane in occluding_finite_plane_caches[player_i]:  
                if (intersects(left_LOS_segment, plane) or
                    intersects(right_LOS_segment, plane)
                   ):
                    to_hide[player_i, enemy_i] = true
                    break       # break out of two layers of loops
            else:               # for ... else statement to break out of outer loop
                enemy_queue_2.push(enemy_i)
                continue
            break
        for object in occluding_objects:  
            if potentially_visible(player, object):  
                occluding_finite_plane_queue.push(get_wall_segment(player, object)) 
        for enemy_i in enemy_queue_2:  
            left_LOS_segment, right_LOS_segment = get_LOS_segments(player_i, enemy_i)
            for plane in occluding_finite_plane_queue:  
                if (intersects(left_LOS_segment, plane) or
                    intersects(right_LOS_segment, plane)
                   ):
                    to_hide[player_i, enemy_i] = true  
                    update_cache_LRU(occluding_finite_plane_cache, plane)
                    break       # break out of two layers of loops
            else:               # for ... else statement to break out of outer loop
                continue
            break
for player_i, enemy_i in idices(to_hide):
    hide(player_i, enemy_i)
enemy_queue.clear()
enemy_queue_2.clear()
occluding_finite_plane_queue.clear()
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
