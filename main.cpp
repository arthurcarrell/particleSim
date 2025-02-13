/*
Particle simulation using SDL

*/


#include <functional>
#include <unordered_map>
#define SDL_MAIN_USE_CALLBACKS 1 /* run SDL_AppInit instead of main() */

#include <cstdlib>
#include <unistd.h>

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <vector>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_timer.h>
#include <iostream>

#include "core/utils.hpp"
#include "core/particle_def.hpp"

using namespace std;


const char PROJECT_Version[] = "1.0"; // project version; self explanatory.
const char PROJECT_ProjectName[] = "com.asteristired.particle_sim"; // the internal name of the project, no spaces or special characters.
const char PROJECT_AppName[] = "Particle Simulation!"; // the name that appears on the window.

float msElapsed = 0;
unsigned long framesDrawn = 0;


namespace ParticleSim {

    /* GLOBAL VARIABLES */
    vector<Particle> scene;

    const int CELL_SIZE = 30;
    unordered_map<int, vector<Particle* >> hashmap_grid;

    int particleBoundsX = 1280;
    int particleBoundsY = 720;
    int nextFreeID = 0;

    /* Runs upon program start */
    void Init(void **appstate, int argc, char *argv[]) {

        // populate the scene with 10 particles
        for (int particleCount=0; particleCount < 1000; particleCount++) {
            // define a new particle
            Particle newParticle;

            // give the particle an ID and increment the ID counter to prevent duplicates.
            newParticle.id = nextFreeID;
            nextFreeID++;

            newParticle.x = rand() % particleBoundsX;
            newParticle.y = rand() % particleBoundsY;
            
            newParticle.velocity = 0.1;
            newParticle.direction = rand() % 360;
            newParticle.type = rand() % 2;
            if (newParticle.type == 1) {
                newParticle.color = {255,0,0, SDL_ALPHA_OPAQUE};
            } else if (newParticle.type == 0) {
                newParticle.color = {0,0,255,SDL_ALPHA_OPAQUE};
            } else {
                newParticle.color = {255,255,255,SDL_ALPHA_OPAQUE};
            }



            SDL_Log("created particle at X: %f, Y: %f error: %s", newParticle.x, newParticle.y, SDL_GetError());
            scene.push_back(newParticle);
        }
        SDL_Log("Particles created! Amount: %lu", scene.size());
    }


    /* --- Optimisation 1: SPATIAL HASHING --- */

    /* Get the cellID of the current particle. */
    int SH_GetCellID(int x, int y) {
        int cellX = x / CELL_SIZE;
        int cellY = y / CELL_SIZE;

        // the grid needs to be < 10000 wide.
        return cellX * 10000 + cellY;
    }

    /* Clear the hashmap */
    void SH_ClearGrid() {
        hashmap_grid.clear();
    }

    /* Insert particle into the hashmap */
    void SH_InsertInGrid(Particle* particle) {
        int cellID = SH_GetCellID(particle->x, particle->y);
        hashmap_grid[cellID].push_back(particle);
    }


    /* Populate the hashmap with all the particles. */
    void SH_PopulateGrid() {
        SH_ClearGrid();

        for (Particle& particle : scene) {
            SH_InsertInGrid(&particle);
        }
    }

    /* Returns a list of pointers to particles  */
    vector<Particle*> SH_GetNearbyParticles(Particle *particle) {
        // this is the vector of all the neighbors, this is going to be used instead of every single particle.
        vector<Particle*> neighbors;

        int cellX = particle->x / CELL_SIZE;
        int cellY = particle->y / CELL_SIZE;

        /* Ok I'll admit I got some help from ChatGPT on how to do this. */
        for (int dx = -1; dx <= 1; dx++) {
            for (int dy = -1; dy <= 1; dy++) {
                int neighborID = SH_GetCellID((cellX + dx) * CELL_SIZE, (cellY + dy) * CELL_SIZE);

                // add particles from neighboring cells
                if (hashmap_grid.find(neighborID) != hashmap_grid.end()) {
                    neighbors.insert(neighbors.end(), hashmap_grid[neighborID].begin(), hashmap_grid[neighborID].end());
                }
            }
        }

        return neighbors;
    }


    Color AvgColours(Color firstColor, Color secondColor) {
        /* 
        NOTES:
        if we have one color that is 255, 0, 0
        and another color that is 0, 0, 255
        it should be about
        128, 0, 128 for both upon hit.
         */
        Color finalColor = {0,0,0, SDL_ALPHA_OPAQUE};
        finalColor.r += firstColor.r / 2;
        finalColor.g += firstColor.g / 2;
        finalColor.b += firstColor.b / 2;
        
        finalColor.r += secondColor.r / 2;
        finalColor.g += secondColor.g / 2;
        finalColor.b += secondColor.b / 2;

        return finalColor;
        
    }

    void DoBounceCheck(Particle *particle) {
        /* Do bouncing if we are hitting the edge of the screen. 
           hasAlreadyBounced is used to prevent the particles hitting themselves twice and bouncing away.
        */
        /* Check if bounced on the X coordinate */
        if (particle->x >= particleBoundsX || particle->x <= 0) {
            if (!particle->hasAlreadyBounced) {
                /* Yes, flip it 180 degrees */
                particle->direction = 180 - particle->direction;
                particle->hasAlreadyBounced = true;
            }
        }

        /* Check if bounced on the Y coordinate */
        if (particle->y >= particleBoundsY || particle->y <= 0) {
            if (!particle->hasAlreadyBounced) {
                /* Yes, reverse the direction */
                particle->direction = -particle->direction;
                particle->hasAlreadyBounced = true;
            }
        }
    }

    void Sim1_ColorAdding(void *appstate, Particle *particle, float deltaTime) {
        // loop through EVERY other particle and see if they are touching us - this gives each frame a time complexity of O(n^2) which is BAD.
        for (int particleIndex = 0; particleIndex < scene.size(); particleIndex++ ) {
            Particle *thisParticle = &scene[particleIndex];
            // If thisParticle and particle are the same, then we are looking at ourselves, skip.
            if (thisParticle->id != particle->id ) { 
                float distance = particle->GetParticleDistance(thisParticle->x,thisParticle->y);
                if (distance < particle->size || distance < thisParticle->size) {
                    // the particle is being touched.
                    thisParticle->color = AvgColours(particle->color, thisParticle->color);
                }

            }
        }
    }

    /* Terrible name, the function for each particle to run when compared to another in the second simulation */
    void Sim2_Func_effectParticle(Particle *particle, Particle *thisParticle, float deltaTime) {

        float distance = particle->GetParticleDistance(thisParticle->x,thisParticle->y);
        float radianAttractDir = particle->GetDirectionOfOtherParticle(thisParticle->x, thisParticle->y);
                
        // calculate the speed of attraction
        particle->velocity = (1 / distance) * 0.1;
        if (particle->velocity > 1) {particle->velocity = 0;}

        // calculate the force of repulsion (this is used to prevent them from crashing into another)
        if (distance <= (particle->size + thisParticle->size)) {
            particle->velocity = -(particle->velocity * 0.5);
        }

        if (particle->type != thisParticle->type) {
            // turn the force of attraction into one of repulsion
            particle->velocity = particle->velocity * -1;
        }


        particle->MoveDirection(particle->velocity * deltaTime, radianAttractDir, true);
        DoBounceCheck(particle);
    }

    /* The original code for the simulation, this is with no optimisations */
    void Sim2_ColorAttraction(void *appstate, Particle *particle, float deltaTime) {

        /* 
            Colors are attracted to colors of the same type, and are repelled by colors of the opposite type.
         */

        // loop through EVERY other particle and see if they are touching us - this gives each frame a time complexity of O(n^2) which is BAD.
        for (int particleIndex = 0; particleIndex < scene.size(); particleIndex++ ) {
            Particle *thisParticle = &scene[particleIndex];
            // If thisParticle and particle are the same, then we are looking at ourselves, skip.
            if (thisParticle->id != particle->id ) { 
                Sim2_Func_effectParticle(particle, thisParticle, deltaTime);
            }
        }
    }

    /* The first optimised simulation using spatial hashing. All functions with a SH_ at the start are to do with this. */
    void Sim2_ColorAttraction_FirstOptimise(void *appstate, Particle *particle, float deltaTime) {
        vector<Particle*> nearbyParticles = SH_GetNearbyParticles(particle);
        for (Particle *thisParticle : nearbyParticles) {
            if (particle->id != thisParticle->id) {
                Sim2_Func_effectParticle(particle, thisParticle, deltaTime);
            }     
        }
        
    }

    void SimParticle(void *appstate, Particle *particle, float deltaTime) {

        //particle->MoveDirection(particle->velocity * deltaTime, particle->direction);
        //DoBounceCheck(particle); // this should always be ran after MoveDirection();

        if (particle->x > 0 && particle->x < particleBoundsX && particle->y > 0 && particle->y < particleBoundsY) {
            particle->hasAlreadyBounced = false;
        }

        Sim2_ColorAttraction_FirstOptimise(appstate, particle, deltaTime);

    }

    /* Runs each frame. When doing a physics calculation, make sure to multiply by deltaTime so framerate doesnt cause wacky numbers */
    void Frame(void *appstate, float deltaTime) {
        framesDrawn++;

        SH_ClearGrid();
        for (Particle &addParticle : scene) {
            SH_InsertInGrid(&addParticle);
        }

        for (int particleIndex = 0; particleIndex < scene.size(); particleIndex++ ) {

            // this particle
            SimParticle(appstate, &scene[particleIndex], deltaTime);
        }
        
        //SDL_Log("Time since last frame (ms): %f  - framerate estimate: %i", deltaTime, (int)(1000/deltaTime));
        msElapsed += deltaTime;
    }

}


namespace Rendering {

    const int WINDOW_Width = 1280;
    const int WINDOW_Height = 720;

    static SDL_Window *window = NULL;
    static SDL_Renderer *renderer = NULL;



    /* NON-CORE FUNCTIONS */

    /* Draw a hollow circle */
    void DrawCircle(SDL_Renderer *renderer, Vec2f coordinates, int radius) {

        // little optimisation, if the radius is 1, then we are only drawing a single pixel. 
        if (radius == 1) {
            SDL_RenderPoint(renderer, coordinates.x, coordinates.y);
            return;
        }

        int diameter = (radius * 2);
        int x = (radius - 1);
        int y = 0;
        int tx = 1;
        int ty = 1;

        int error = (tx - diameter);

        while (x >= y) {
            // Each of the following renders an octant of the circle
            SDL_RenderPoint(renderer, coordinates.x + x, coordinates.y - y);
            SDL_RenderPoint(renderer, coordinates.x + x, coordinates.y + y);
            SDL_RenderPoint(renderer, coordinates.x - x, coordinates.y - y);
            SDL_RenderPoint(renderer, coordinates.x - x, coordinates.y + y);
            SDL_RenderPoint(renderer, coordinates.x + y, coordinates.y - x);
            SDL_RenderPoint(renderer, coordinates.x + y, coordinates.y + x);
            SDL_RenderPoint(renderer, coordinates.x - y, coordinates.y - x);
            SDL_RenderPoint(renderer, coordinates.x - y, coordinates.y + x);

            if (error <= 0) {
                ++y;
                error += ty;
                ty += 2;
            }

            if (error > 0) {
                --x;
                tx += 2;
                error += (tx - diameter);
            }
        }
    }

    void DrawForEachParticle(SDL_Renderer *renderer) {
        for (int particleIndex=0; particleIndex < ParticleSim::scene.size(); particleIndex++ ) {
            
            Particle currentParticle = ParticleSim::scene[particleIndex];
            Vec2f currentCoordinates = {currentParticle.x, currentParticle.y};
    
            SDL_SetRenderDrawColor(renderer, currentParticle.color.r, currentParticle.color.g, currentParticle.color.b, SDL_ALPHA_OPAQUE);

            DrawCircle(renderer, currentCoordinates, currentParticle.size);
        }
    }

    /*  === CORE FUNCTIONS === */

    /* Runs on program start */
    SDL_AppResult Init(void **appstate, int argc, char *argv[]) {
        /*
        Effectively just set everything up,
        if any part fails to run, return SDL_APP_FAILURE.
        if not, continue the script, and run SDL_APP_CONTINUE.
        */

        SDL_SetAppMetadata(PROJECT_AppName,PROJECT_Version,PROJECT_ProjectName);

        // init video
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            // something has broke.
            SDL_Log("Couldn't Initalise SDL: %s", SDL_GetError());
            return SDL_APP_FAILURE;
        }

        // create the window and the renderer
        if (!SDL_CreateWindowAndRenderer(PROJECT_AppName, WINDOW_Width, WINDOW_Height, 0, &window, &renderer)) {
            SDL_Log("Couldn't create window/renderer: %s", SDL_GetError());
            return SDL_APP_FAILURE;

        }

        return SDL_APP_CONTINUE;
    }

    /* Runs each frame */
    SDL_AppResult Frame(void *appstate) {
        SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE_FLOAT);

        /* clear the window to the draw colour */
        SDL_RenderClear(renderer);

        DrawForEachParticle(renderer);
        
        /* place the new render onto the screen */
        SDL_RenderPresent(renderer);

        // success, continue the program.
        return SDL_APP_CONTINUE;
    }
}



/* Runs on Startup */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[]) {

    // do rendering
    Rendering::Init(appstate, argc, argv);

    // create random
    srand(time(NULL));
    // everything has been properly intialised. continue with the program
    ParticleSim::Init(appstate, argc, argv);
    return SDL_APP_CONTINUE;
}

/* runs on I/O interrupt. This could be a keypress or a mouse click. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        // a key that means quit the program has been pressed. (my example: SUPER+SHIFT+Q)
        return SDL_APP_SUCCESS; // quit the program, but in an intentional way, we are not quitting due to an error.
    }
    return SDL_APP_CONTINUE; // key was not a quit key, ignore and continue.
}



/* Runs once per frame, the meat and potatoes of the program. */
long NOW = SDL_GetPerformanceCounter();
long LAST = 0;
double calcDeltaTime = 0;
SDL_AppResult SDL_AppIterate(void *appstate) {

    LAST = NOW;
    NOW = SDL_GetPerformanceCounter();
    calcDeltaTime = ((NOW - LAST)*1000 / (double)SDL_GetPerformanceFrequency());
    ParticleSim::Frame(appstate, calcDeltaTime);

    Rendering::Frame(appstate);

    // success, continue the program.
    return SDL_APP_CONTINUE;

}

/* Runs on shutdown */
void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    /* SDL cleans up window/renderer */
    SDL_Log("average frame render time: %f - average FPS: %i", (msElapsed/framesDrawn), (int)(1000/(msElapsed/framesDrawn)));
}

int main() {
    cout << "Hello World!";
    return 1;
}