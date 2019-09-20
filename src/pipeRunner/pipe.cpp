#include "pipe.h"

void Pipe::generate_points(Vector v, int start){

    float a,b,c;
    Vector v1,v2;
    int angle;
    Vector alea;
    Transform t;

    // Genere 10 nouveaux points
    for(int i =start; i<10; i++) {
        a = rand()%21;
        b = rand()%21;
        c = rand()%21;
        // angle aleatoire
        angle = rand()%20+5; // 40+30
        // sur un axe aleatoire
        a = -1 + a/10;
        b = -1 + b/10;
        c = -1 + c/10;

        alea = Vector(a,b,c);
        v = Rotation(alea,angle)(v);

        Point final = broken_curve_points[i]+ v;
        broken_curve_points.push_back( Point( final ) );
    }
    // On recupere les 2 derniers pour re-generer plus tard
    lastBrokenPoint_minus1=broken_curve_points[broken_curve_points.size()-1];
    lastBrokenPoint_minus2=broken_curve_points[broken_curve_points.size()-2];
}

Mesh Pipe::progressive_pipe(int diff){
    // Genere un nouveau mesh de pipe_parts_number cylindres
    Mesh actual_pipe(GL_TRIANGLES);

    for(int i=0; i<pipe_parts_number; i++) {
        for(int j=0; j<parts; j++) {
            Vector n;
            n= normalize(Vector(pipe_points[i][j]-curve_points[i]));
            actual_pipe.normal(n).texcoord(vec2(0,0)).vertex(pipe_points[i][j]);
            n= normalize(Vector(pipe_points[i+1][(j+1)%parts]-curve_points[i+1]));
            actual_pipe.normal(n).texcoord(vec2(1,1)).vertex(pipe_points[i+1][(j+1)%parts]);
            n= normalize(Vector(pipe_points[i+1][j]-curve_points[i+1]));
            actual_pipe.normal(n).texcoord(vec2(1,0)).vertex(pipe_points[i+1][j]);

            n= normalize(Vector(pipe_points[i][j]-curve_points[i]));
            actual_pipe.normal(n).texcoord(vec2(0,0)).vertex(pipe_points[i][j]);
            n= normalize(Vector(pipe_points[i][(j+1)%parts]-curve_points[i]));
            actual_pipe.normal(n).texcoord(vec2(0,1)).vertex(pipe_points[i][(j+1)%parts]);
            n = normalize(Vector(pipe_points[i+1][(j+1)%parts]-curve_points[i+1]));
            actual_pipe.normal(n).texcoord(vec2(1,1)).vertex(pipe_points[i+1][(j+1)%parts]);
        }
    }

    // Enleve diff points/rotations
    for(int i=0; i<diff; i++){
        pipe_points.erase(pipe_points.begin()+i);
        curve_points.erase(curve_points.begin()+i);
        curve_rotations.erase(curve_rotations.begin()+i);
    }

    return actual_pipe;
}

Point Pipe::init_vec_rotate(){
    // Cree la premiere normale a la courbe
    Vector d;
    Vector vd(0,-1,0);         // vd et a non colineaires !
    Vector a = curve_points[1] - curve_points[0];

    d = cross(a,vd);
    if (d.x==0 && d.y==0 && d.z==0) {
        // Verification de parallélisme
        vd= Vector(-1,0,0);
        d = cross(a,vd);
    }

    d = normalize(d);
    vectortmp = d;
    d = d * 0.035;
    // valeur arbitraire ici pour le diametre du tuyau

    Point pt_d = Point(d);

    std::vector<Point> pipePoints;

    // On cree son cercle
    for(int i=0; i<parts; i++)
    pipePoints.push_back( Translation(Vector(curve_points[0])) (Rotation(a, i * 360/parts) (pt_d)) );

    pipe_points.push_back(pipePoints);

    return pt_d;
}

void Pipe::curve_algorithm(int steps){
    // Lisse un courbe en steps etapes
    std::vector<Point> vec_vec;

    for(int k=0; k<steps; k++) {
        vec_vec = std::vector<Point>();
        for(int i=0; (unsigned)i<broken_curve_points.size()-1; i++) {
            // Divise chaque angle en 2
            vec_vec.push_back(Point(broken_curve_points[i]+0.25*(broken_curve_points[i+1]-broken_curve_points[i])));
            vec_vec.push_back(Point(broken_curve_points[i]+0.75*(broken_curve_points[i+1]-broken_curve_points[i])));
        }
        // On récupère les nouveaux points
        broken_curve_points = vec_vec;
    }
    for(int i=0; (unsigned)i<broken_curve_points.size(); i++){
        curve_points.push_back(broken_curve_points[i]);
    }
}

void Pipe::rotations_computing(int start){
    // Calcul des matrices de rotation a -> b de chaque segment
    // a appliquer sur le vecteur orthogonal de chaque point
    Vector a,b;
    Transform r;

    for(int k=start; (unsigned)k<curve_points.size()-1; k++) {

        a = normalize(curve_points[k] - curve_points[k-1]);
        b = normalize(curve_points[k+1] - curve_points[k]);

        r = getRotationBetweenVectors(a,b);
        curve_rotations.push_back(r);
    }

}

void Pipe::curvePoints_computing(int start, Point pt_d){
    // Calcule tous les cercles de points en chaque point de la courbe
    std::vector<Point> pipePoints;
    Vector a;
    for(int k=start; (unsigned)k<curve_rotations.size(); k++) {
        pipePoints = std::vector<Point>();
        a = normalize(curve_points[k+1] - curve_points[k]);
        pt_d = curve_rotations[k](pt_d);
        for(int i=0; i<parts; i++)
        pipePoints.push_back( Translation(Vector(curve_points[k+1])) (Rotation(a, i * 360/parts) (pt_d)) );

        pipe_points.push_back(pipePoints);
    }
    final_ptd = pt_d;
}

void Pipe::curveMesh_computing(int start){
    // Calcule tous les triangles du tuyau
    for(int i=start; i<pipe_parts_number; i++) {         //(unsigned)i<pipe_points.size()-1
        for(int j=0; j<parts; j++) {

            Vector n;
            n= normalize(Vector(pipe_points[i][j]-curve_points[i]));
            pipe.normal(n).texcoord(vec2(0,0)).vertex(pipe_points[i][j]);
            n= normalize(Vector(pipe_points[i+1][(j+1)%parts]-curve_points[i+1]));
            pipe.normal(n).texcoord(vec2(1,1)).vertex(pipe_points[i+1][(j+1)%parts]);
            n= normalize(Vector(pipe_points[i+1][j]-curve_points[i+1]));
            pipe.normal(n).texcoord(vec2(1,0)).vertex(pipe_points[i+1][j]);

            n= normalize(Vector(pipe_points[i][j]-curve_points[i]));
            pipe.normal(n).texcoord(vec2(0,0)).vertex(pipe_points[i][j]);
            n= normalize(Vector(pipe_points[i][(j+1)%parts]-curve_points[i]));
            pipe.normal(n).texcoord(vec2(0,1)).vertex(pipe_points[i][(j+1)%parts]);
            n = normalize(Vector(pipe_points[i+1][(j+1)%parts]-curve_points[i+1]));
            pipe.normal(n).texcoord(vec2(1,1)).vertex(pipe_points[i+1][(j+1)%parts]);
        }
    }
}

void Pipe::genPipe(){
    // Genere un nouveau morceau de Tuyau

    // Recuperation des anciens 2 derniers points
    broken_curve_points = std::vector<Point>();
    broken_curve_points.push_back(lastBrokenPoint_minus1);
    Vector v1(lastBrokenPoint_minus1-lastBrokenPoint_minus2);
    broken_curve_points.push_back(lastBrokenPoint_minus1+v1);

    // On regenere
    generate_points(v1,1);

    int nbpoint = curve_points.size();
    int nbpipepoints = pipe_points.size();
    // On lisse
    curve_algorithm(curveSmooth);
    Vector entre2( curve_points[nbpoint] - curve_points[nbpoint-1]);

    // ECART A COMBLER de notre algo
    int steps = length(entre2) / length(Vector(   curve_points[nbpoint-1] - curve_points[nbpoint-2]));
    steps = steps -1 ;
    for(int i =0; i<steps-1; i++){
        //begin =0 //pour pas avoir d'acoup en affichage progressif
        // points equidistants
        curve_points.insert(curve_points.begin()+(nbpoint+i), Point( curve_points[nbpoint-1] + (i+1) * (entre2/steps) ) );
    }
    rotations_computing(nbpoint-1);
    // indice du dernier point a la rotation non effectuee > dernier point de la courbe precedente
    curvePoints_computing(nbpoint-2 ,final_ptd);

    // maj du vectortmp
    vectortmp = normalize(Vector(pipe_points[nbpoint-2][0]-curve_points[nbpoint-2]));

}

void Pipe::pipe_generation(){

    // ajout mannuel d'un debut de tuyau droit
    broken_curve_points.push_back(Point(0,0,0));
    broken_curve_points.push_back(Point(0,0,0.5));
    broken_curve_points.push_back(Point(0,0,1));
    broken_curve_points.push_back(Point(0,0,1.5));
    broken_curve_points.push_back(Point(0,0,2));
    broken_curve_points.push_back(Point(0,0,2.5));
    broken_curve_points.push_back(Point(0,0,3));
    broken_curve_points.push_back(Point(0,0,3.5));
    broken_curve_points.push_back(Point(0,0,4));
    generate_points(Vector(0,0,0.5),8);

    // execution de 2 genPipe pour combler le debut plus facile
    curve_algorithm(curveSmooth);

    rotations_computing(1);
    Point pt_d = init_vec_rotate();
    curvePoints_computing(0 ,pt_d);

    broken_curve_points = std::vector<Point>();
    broken_curve_points.push_back(lastBrokenPoint_minus1);
    Vector v1(lastBrokenPoint_minus1-lastBrokenPoint_minus2);
    broken_curve_points.push_back(lastBrokenPoint_minus1+v1);

    generate_points(v1,1);
    int nbpoint = curve_points.size();

    curve_algorithm(curveSmooth);

    Vector entre2( curve_points[nbpoint] - curve_points[nbpoint-1]);

    // ECART A COMBLER de notre algo
    int steps = length(entre2) / length(Vector(   curve_points[nbpoint-1] - curve_points[nbpoint-2]));
    steps = steps -1 ; // logique
    for(int i =0; i<steps-1; i++){
        //begin =0 //pour pas avoir d'acoup en affichage progressif
        curve_points.insert(curve_points.begin()+(nbpoint+i), Point( curve_points[nbpoint-1] + (i+1) * (entre2/steps) ) );
    }

    rotations_computing(nbpoint-1); // indice du dernier point a la rotation non effectuee > dernier point de la courbe precedente
    curvePoints_computing(nbpoint-2 ,final_ptd);
    // curveMesh_computing(nbpoint-2);
    curveMesh_computing(0);
}

Vector Pipe::getVectorTmp(){
    return vectortmp;
}

void Pipe::resetVectors(){
    broken_curve_points = std::vector<Point>();
    curve_points= std::vector<Point>();
    curve_rotations = std::vector<Transform>();
    pipe_points= std::vector<std::vector<Point> >();
}

Transform getRotationBetweenVectors(Vector a, Vector b){
    // explicite
    Vector v;
    float c,lastpart;
    Transform r,vx,vx2,id = Identity();

    v = cross(a,b);
    c = dot(a,b);

    vx = Transform( 0, -v.z, v.y, 0, v.z, 0, -v.x, 0, -v.y, v.x, 0, 0, 0, 0, 0, 1 );

    lastpart =  1  / ( 1 + c );
    vx2 = vx(vx);

    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            r.m[i][j] = id.m[i][j] + vx.m[i][j] + vx2.m[i][j] * lastpart;
        }
    }

    return r;
}
