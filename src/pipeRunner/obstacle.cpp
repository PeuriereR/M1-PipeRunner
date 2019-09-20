#include "obstacle.h"

Obstacle::Obstacle(const char *filename){
    obstacleMesh = read_mesh(filename);
    obstacleMesh.bounds(meshPmin,meshPmax);
}

void Obstacle::addModel(Transform T){
    modelsVector.push_back(T);
}

void Obstacle::popModel(){
    modelsVector.erase(modelsVector.begin());
}

bool Obstacle::collision(Point p1, Point p2, Transform T){
    for(int i=0; i<3; i++){
        if ( collides(meshPmin, meshPmax, modelsVector[i], p1, p2, T) ){
            return true;
        }
    }
    return false;
}

Transform& Obstacle::getModel(int i){
    return modelsVector[i];
}
Mesh& Obstacle::getMesh(){
    return obstacleMesh;
}

void Obstacle::getBounds(Point& p1, Point& p2){
    p1 = meshPmin;
    p2 = meshPmax;
}

bool collision_algo(Vector direction, Point p_min, Point p_max, Point rhs_pmin, Point rhs_pmax, Transform T_this, Transform T_rhs){

    bool min = (direction.x + direction.y + direction.z < 0) ? true : false;
    bool x = (direction.x != 0) ? true : false;
    bool y = (direction.y != 0) ? true  : false;

    direction = T_this(direction);              // On passe du repere this au repere world
    direction = T_rhs.inverse() (direction);    // On passe du repere world au repere rhs

    Point coin;
    // On determine le coin le plus proche

    if (direction.x >= 0)
    coin.x = rhs_pmin.x;
    else coin.x = rhs_pmax.x;
    if (direction.y >= 0)
    coin.y = rhs_pmin.y;
    else coin.y = rhs_pmax.y;
    if (direction.z >= 0)
    coin.z = rhs_pmin.z;
    else coin.z = rhs_pmax.z;

    coin = T_rhs(coin);                         // On passe du repere rhs au repere world
    coin = T_this.inverse()(coin);             // On pase du repere world au repere this

    if (min) {
        if (x) {
            if (coin.x <= p_min.x)
            return true;
        }
        else if (y) {
            if (coin.y <= p_min.y)
            return true;
        }
        else if (coin.z <= p_min.z)
        return true;
    }
    else{
        if (x) {
            if (coin.x >= p_max.x)
            return true;
        }
        else if (y) {
            if (coin.y >= p_max.y)
            return true;
        }
        else if (coin.z >= p_max.z)
        return true;
    }

    return false;
}

bool collides(Point pmin, Point pmax, Transform T, Point rhspmin, Point rhspmax, Transform rhsT) {

    // 6 Faces de This
    if (collision_algo(Vector(0,-1,0),pmin,pmax,rhspmin,rhspmax,T,rhsT))
    return false;
    if (collision_algo(Vector(0,1,0),pmin,pmax,rhspmin,rhspmax,T,rhsT))
    return false;
    if (collision_algo(Vector(-1,0,0),pmin,pmax,rhspmin,rhspmax,T,rhsT))
    return false;
    if (collision_algo(Vector(1,0,0),pmin,pmax,rhspmin,rhspmax,T,rhsT))
    return false;
    if (collision_algo(Vector(0,0,-1),pmin,pmax,rhspmin,rhspmax,T,rhsT))
    return false;
    if (collision_algo(Vector(0,0,1),pmin,pmax,rhspmin,rhspmax,T,rhsT))
    return false;

    return true;
}
