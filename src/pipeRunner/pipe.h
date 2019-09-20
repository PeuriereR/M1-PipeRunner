#include <vector>
#include "mesh.h"
#include "mat.h"

/*
 * Gere la creation, MAJ d'un tuyau
 */

class Pipe{
public:
    Pipe(){};
    Pipe(int partsNumber, int curveSmoothness, int cylinderSmoothness): curveSmooth(curveSmoothness),parts(cylinderSmoothness),pipe_parts_number(partsNumber),pipe(GL_TRIANGLES){};

    /* Genere les points de maniere aleatoire dirigée par le Vecteur */
    void generate_points(Vector , int);
    /* Initialise la première normale à la courbe */
    Point init_vec_rotate();
    /* Fait évoluer pipe suivant diff et pipe_parts_number */
    Mesh progressive_pipe(int);
    /* Calcule les rotations entre les normales les segments */
    void rotations_computing(int);
    /* Première génération (appel aux autres fonctions) de pipe (plat au debut) */
    void pipe_generation();
    /* Lisse la courbe */
    void curve_algorithm(int);
    /* Calcule toutes les courbes formant le tuyau */
    void curvePoints_computing(int , Point);
    /* Utilise pipe_points pour créer le mesh */
    void curveMesh_computing(int);
    /* Re-génère une partie de tuyau */
    void genPipe();
    /* Réinitialise les vecteurs */
    void resetVectors();
    /* Retourne vectortmp */
    Vector getVectorTmp();

private:
    // Derniers points utilisés pour la courbe (pour re-generer)
    Point lastBrokenPoint_minus1;
    Point lastBrokenPoint_minus2;
    // Dernier point lissé
    Point final_ptd;
    // Nombre d'itération de curve_algorithm
    int curveSmooth;
    // Nombre de faces au tuyau
    int parts;
    // Nombre de segments (cylindres) du tuyau
    int pipe_parts_number;
    // Vecteur des points de la courbe non lissée
    std::vector<Point> broken_curve_points;
    // Première normale au segment de tuyau actuel
    Vector vectortmp;
public:
    // Points de la courbe lissée (nbpt)
    std::vector<Point> curve_points;
    // Rotations de la courbe lissée
    std::vector<Transform> curve_rotations;
    // Points (vertex) du tuyau
    std::vector<std::vector<Point> > pipe_points;
    // Tuyau
    Mesh pipe;
};

/* Retourne la matrice de rotation entre deux vecteurs */
Transform getRotationBetweenVectors(Vector, Vector);
