#include <vector>
#include "mat.h"
#include "mesh.h"
#include "wavefront.h"

/*
 * Gere une liste d'obstacles (models) ayant le meme mesh
 */

class Obstacle{
public:
    Obstacle(){};
    // Crée une liste d'obstacle sur un .obj
    Obstacle(const char*);
    // Ajoute un obstacle
    void addModel(Transform);
    // Enleve un obstacle
    void popModel();
    // Check toutes les collisions avec les models
    bool collision(Point, Point, Transform);
    // Retourne un model précis
    Transform& getModel(int);
    // Retourne le mesh de l'obstacle
    Mesh& getMesh();
    // Retourne la boite englobante de l'obstacle
    void getBounds(Point& , Point&);

    inline int modelsCount(){ return modelsVector.size(); };
    inline void resetModels(){ modelsVector = std::vector<Transform>(); };

private:
    std::vector<Transform> modelsVector;
    Mesh obstacleMesh;

    Point meshPmin;
    Point meshPmax;
};

// Algorithme de collision de boites englobantes
bool collision_algo(Vector , Point , Point , Point , Point , Transform , Transform );
bool collides(Point , Point , Transform , Point , Point , Transform );
