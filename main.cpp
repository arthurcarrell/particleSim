/*
Particle simulation using SDL

*/

#include <cstdlib>
#include <unistd.h>
#define SDL_MAIN_USE_CALLBACKS 1 /* run SDL_AppInit instead of main() */


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

/* GLOBAL VARIABLES */


namespace ParticleSim {

    /* GLOBAL VARIABLES */
    vector<Particle> scene;
    unsigned long framesElasped;

    int particleBoundsX = 1280;
    int particleBoundsY = 720;

    /* Runs upon program start */
    void Init(void **appstate, int argc, char *argv[]) {

        // populate the scene with 10 particles
        for (int particleCount=0; particleCount < 72; particleCount++) {
            // define a new particle
            Particle newParticle;
            //newParticle.x = rand() % particleBoundsX;
            //newParticle.y = rand() % particleBoundsY;
            newParticle.x = particleBoundsX / 2;
            newParticle.y = particleBoundsY / 2;
            
            newParticle.velocity = 0.1;
            newParticle.direction = particleCount*5;
            newParticle.color = {255,255,255, SDL_ALPHA_OPAQUE};


            if (newParticle.direction == 90) { newParticle.color = {255,0,0, SDL_ALPHA_OPAQUE}; }

            SDL_Log("created particle at X: %f, Y: %f error: %s", newParticle.x, newParticle.y, SDL_GetError());
            scene.push_back(newParticle);
        }
        SDL_Log("Particles created! Amount: %lu", scene.size());
    }

    void SimParticle(void *appstate, Particle *particle, float deltaTime) {
        particle->MoveDirection(particle->velocity * deltaTime, particle->direction);

    }
    /* Runs each frame. When doing a physics calculation, make sure to multiply by deltaTime so framerate doesnt cause wacky numbers */
    void Frame(void *appstate, float deltaTime) {
        framesElasped++;
        // for each particle
        for (int particleIndex = 0; particleIndex < scene.size(); particleIndex++ ) {

            // this particle
            SimParticle(appstate, &scene[particleIndex], deltaTime);

        }
        if (framesElasped == 10000) {
                SDL_Delay(1000);
        }
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
}

int main() {
    cout << "Hello World!";
    return 1;
}