# CornerCulling
Fast, maximally accurate, and latency resistant culling method.  
Proof of concept in C++ and UE4. Wallhack Penicillin.  
Calculates lines of sight from possible (due to latency) locations of players to the corners of the bounding volumes of enemies,
determining if they intersect with occluding objects.
#### Latest Demo
https://youtu.be/K8jm5evnPiY

An alternative implementation could fetch all objects potentially along each line of sight,
using an efficient bounding volume hierarchy. In a game with a large number of static objects,
this method's logarithmic complexity could result in huge speedups.

## Non-Technical Pitch

While not as rage-inducing as a blatant aim-botter, a wallhacker inflicts great mental strain on their victims, making them constantly uneasy and suspicious. Many serious FPS players, including myself, have spent hours staring at replays, trying to catch them red-handed. But at some point, it all becomes too exhausting.

Furthermore, wallhacks are often too subtle to detect with simple heuristics, making detection and punishment strategies costly and ineffective.

So, instead of detecting wallhacks, some games have adopted a prevention strategy. With this strategy, the game server independently calculates if a player can see an enemy. If the player cannot see the enemy, then the server does not send the enemy location to the player. Even if a malicious player tampers with their game, they will not be able to find information that they did not receive.

Unfortunately, modern implementations of this idea are inaccurate or slow. I have developed a solution that is both perfectly accurate and faster than currently deployed solutions--utilizing ideas developed in the past 25 years of graphics research.

## Technical Details

Instead of using slow ray marches or fast approximations like PVS, we geometrically calculate if each potential line of sight is blocked by large, occluding polyhedra in the scene. A huge speed gain comes from caching recent polyhedra--if a corner blocked a line of sight a few milliseconds ago, it is likely to block the same line of sight now. If so, we can skip checking all other polyhedra. Ideally, the system would also implement a scene hierarchy like Binary Space Partition or Bounding Volume Hierarchy for logarithmic object lookup on cache misses. This improvement is unnecessary for small 5v5 shooters like CS:GO, but it might be for military and battle-royale games. Furthermore, a potentially visible set check between players (but not to bounding volumes if scene hierarchy is implemented) could also increase speed 

Also, we must account for latency to prevent popping. To do so, one should calculate angles not from the player's last known position, but from the most aggressive positions they could peek. A practical method to check all such positions is to, on the plane normal to the line from player to enemy, calculate the four corners of the rectangle that contains all possible peeks. Then let an object cull an enemy only when lines of sight from all four corners are blocked.

By accounting for latency, we can also afford to speed up average culling time by a factor of K if we cull every K ticks. Compared to a 100 ms ping, the added delay of culling every 30 ms instead of 10 ms is minimal--but results in a 3x speedup. I have not tested whether it is ideal to spread out the culling over multiple ticks for all game server instances running on a single CPU, or if it is better to stagger the full culling cycle of each game instance. I suspect that the latter will have better cache performance.

The last big tip is to keep enemies revealed for a few culling cycles. It is expensive when all polyhedra failed to occlude an enemy, especially if many of them barely failed. Keeping enemies revealed for ~200 ms does not confer a big advantage to wallhackers, but could save CPU cycles. This timer can adapt to server load.
               
## Priority Tasks
- Implement bounding volume hierarchy
- Talk to engineers at Umbra

## BVH Design
```python
# Classes
BVH
# Functions
BVH(Cuboids)
BVH.GetPossibleOccludingCuboids(Bundle) 
```

## Other Tasks (in no order):
- Implement potentially visible sets to pre-cull enemies.
- Reach out to graphics experts for review
- Continue researching graphics community state of the art.
- Consider ways to partially occlude enemies, trimming down their bounding boxes.
  Currently, if two objects each occlude 99% of an enemy, the enemy is still visible because a sliver
  of their left is visible to one box, and a sliver of their right is visible to another.
  We would have to implement a polyhedra clipping algorithm, or some discrete approximation of it.
  Alternatively, subdivide one bounding box into many, and cull those individually.
- Consider sending fake enemy locations to throw off cheaters.

## Research

### Occlusion Culling:  
http://www.cs.unc.edu/~zhangh/hom.html  
https://www.gamasutra.com/view/feature/131388/rendering_the_great_outdoors_fast_.php?page=3  
https://medium.com/@Umbra3D/introduction-to-occlusion-culling-3d6cfb195c79  

### Improved bounding boxes (k-dops):  
https://www.youtube.com/watch?v=h4GBU-NXJ1c  

### Faster raytracing:  
Real-Time Rendering, Fourth Edition
http://www0.cs.ucl.ac.uk/staff/j.kautz/teaching/3080/Slides/16_FastRaytrace.pdf
https://www.cs.cmu.edu/afs/cs/academic/class/15462-s09/www/lec/14/lec14.pdf
https://hwrt.cs.utah.edu/papers/hwrt_siggraph07.pdf
http://webhome.cs.uvic.ca/~blob/courses/305/notes/pdf/Ray%20Tracing%20with%20Spatial%20Hierarchies.pdf
https://stackoverflow.com/questions/99796/when-to-use-binary-space-partitioning-quadtree-octree

### Fast geometric intersection algorithms
https://en.wikipedia.org/wiki/Intersection_of_a_polyhedron_with_a_line
https://tavianator.com/cgit/dimension.git/tree/libdimension/bvh/bvh.c#n196
http://paulbourke.net/geometry/circlesphere/index.html#linesphere

### Potentially Useful Geomeotry
https://en.wikipedia.org/wiki/Back-face_culling
https://en.wikipedia.org/wiki/Clipping_(computer_graphics)

### Note on (un) originaliy
Soon after developing my initial prototype, I discovered that my idea is basically shadow culling,
which graphics researchers documented in 1997. <br />  
https://www.gamasutra.com/view/feature/3394/occlusion_culling_algorithms.php?print=1 <br />  
[Coorg97] Coorg, S., and S. Teller, "Real-Time Occlusion Culling for Models with Large Occluders", in Proceedings 1997 Symposium on Interactive 3D Graphics, pp. 83-90, April 1997.  
[Hudson97b] Hudson, T., D. Manocha, J. Cohen, M. Lin, K. Hoff and H. Zhang, "Accelerated Occlusion Culling using Shadow Frusta", Thirteenth ACM Symposium on Computational Geometry, Nice, France, June 1997.  

## Graphics Libraries:  
https://www.cgal.org/  
https://www.geometrictools.com/  
https://docs.unrealengine.com/en-US/API/Runtime/Core/Math/FMath/index.html  
