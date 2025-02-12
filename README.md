# Particle Simulation in SDL3

This is a particle simulation that I wrote in a day in C++ using SDL3.
Its pretty simple, and as of (12/02/25) is incredibly slow. Crawling to about 14 FPS when met with only 1000 particles. This is because it is entirely unoptimised. Currently my simulation has an `O(n^2)` time complexity (very bad).<br>

Now, this project has turned from making a particle simulation to optimising a particle system. My goal is to get 10,000 particles running above 60FPS.

Im going to achieve this by adding:

- Spatial Hashing
- Batch Rendering


### Spatial Hashing
While technically every single particle in the simulation is effected by every other particle, the particles far away are so minimally affected that it can basically be 0. 

The simulation is split into cells, and when checking the particle, we check the cell it is in and ajacent cells around it. This allows us to check *far far* less cells then in a non-optimised approach.

According to \**cough*\* chatGPT this will speed up the program by up to 100 times.

### Batch Rendering
This one is a lot more simple to descirbe and a lot more simple to program. Instead of repeatedly using `SDL_RenderPoint`, which is pretty slow. We take everything we want to draw and we feed it to `SDL_RenderGeometry`.

This should make the program up to 5x faster.





TODO: Add how the particle simulation actually works to this lol.