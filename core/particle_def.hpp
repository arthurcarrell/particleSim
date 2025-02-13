#ifndef PARTICLE_DEF_HPP
#define PARTICLE_DEF_HPP
#include "utils.hpp"
#include <cmath>

class Particle {
    public:
        float x;
        float y;
        int type;
        int id;
        float size = 1;
        float velocity;
        int direction;
        Color color;

        bool hasAlreadyBounced = false;

        // move the particle a certain X/Y value
        void MoveCoords(Vec2 coordinates) {
            this->x += coordinates.x;
            this->y += coordinates.y;
        }

        void MoveDirection(float amount, float angle, bool isRadians=false) {
            // trigonometry can be used in order to calculate diagonal movement
            // sin and cos is used to figure out the hypotonuse.
            // presumes that degrees are being used. if they are: convert to radians

            // if radians are being used, which we presume not, then skip this step.
            if (!isRadians) {
                // convert angle to radians
                double PI = 3.14159265;
                angle = angle * PI/180;
            }
            

            float dx = amount * cos(angle);
            float dy = amount * sin(angle);
            
            this->x += dx;
            this->y += dy;
        }

        float GetParticleDistance(int ox, int oy) {
            // use pythagorus theorem to calculate the hypotenuse
            float answer = sqrt(((ox - this->x) * (ox - this->x)) + ((oy - this->y) * (oy - this->y)));
            return answer;
            
        }

        float GetDirectionOfOtherParticle(int ox, int oy) {
            double dx = ox-x; double dy = oy-y;

            double otherDirection = atan2(dy, dx);

            return otherDirection;
        }
};

#endif