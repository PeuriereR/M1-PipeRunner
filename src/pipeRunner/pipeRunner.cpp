#include "app.h"
#include "mesh.h"
#include "draw.h"
#include "color.h"
#include "mat.h"
#include "wavefront.h"
#include "texture.h"
#include "program.h"
#include "uniforms.h"
#include "obstacle.h"
#include "pipe.h"

class Project : public App {
public:
    Project(int x, int y) : App(x,y), mainRECT(GL_TRIANGLES),activeScoreRECT1(GL_TRIANGLES),activeScoreRECT2(GL_TRIANGLES),finalScoreRECT2(GL_TRIANGLES),finalScoreRECT21(GL_TRIANGLES), activeRotation(0){

        srand(time(NULL));

        // Initialisation des variables
        stop = true;
        startGame = false;
        indiceCP = 1;
        score = 0;
        lost_displacement = 0;
        lost = false;
        torusPlacement = 200;
        spaceshipPlacement = 40;
        parts = 50;
        pipe_parts_number = 300;

        // Creation du tuyau
        pipeTEST = Pipe(pipe_parts_number,5,parts);
        pipeTEST.pipe_generation();
    }

    int render(){

        int diff=0;

        glClear(GL_COLOR_BUFFER_BIT  | GL_DEPTH_BUFFER_BIT);

        // CAMERA de l'ecran de demarrage
        int mx, my;
        unsigned int mb= SDL_GetRelativeMouseState(&mx, &my);
        if(mb & SDL_BUTTON(1))
        starterCamera.rotation(mx, my);  // tourne

        // Restart
        if(key_state('r')){
            clear_key_state('r');

            // On reinitialise les variables
            stop = true;
            lost_displacement = 0;
            lost = false;
            score= 0;
            indiceCP = 1;
            ticks = SDL_GetTicks();

            // On reinitialise le tuyau, et en regenere un
            pipeTEST.resetVectors();
            pipeTEST.pipe_generation();
            pipeTEST.pipe.release();
            pipeTEST.pipe = pipeTEST.progressive_pipe(diff);

            // On reinitialise les CP et obstacles
            cones.resetModels();
            checkpointModels = std::vector<Transform>();

            // On ajoute les nouveaux obstacles
            Transform ObstacleInitialRotation = getRotationBetweenVectors(Vector(0,1,0),normalize(pipeTEST.getVectorTmp()));
            addobstacle(std::min(20,10+score),0,ObstacleInitialRotation);

            // On reinitialise le vaisseau
            Point pmin,pmax;
            Spaceship.bounds(pmin,pmax);

            spaceshipModel = RotationY(-180);
            spaceshipModel = Translation(Vector(0,1.2*(pmax.y-pmin.y),0))(spaceshipModel);
            spaceshipModel = Scale(0.015,0.015,0.015)(spaceshipModel);

            spaceshipModel = ObstacleInitialRotation(spaceshipModel);

            for(int b=0; b<spaceshipPlacement; b++){
                spaceshipModel = pipeTEST.curve_rotations[b](spaceshipModel);
            }

            // On reinitialise le torus
            torus_Part1.bounds(pmin,pmax);

            torusModel = RotationX(90);
            torusModel = Scale(0.1,0.1,0.1)(torusModel);
            torusModel = ObstacleInitialRotation(torusModel);

            for(int b=0; b<torusPlacement; b++){
                torusModel = pipeTEST.curve_rotations[b](torusModel);
            }
        }

        if(key_state('d')){
            // Tourne a droite
            activeRotation++;
            activeRotation = activeRotation % parts;
        }

        if(key_state('q')){
            // Tourne a gauche
            if (activeRotation==0)
            activeRotation = parts;
            activeRotation--;
        }

        //  FOR DEBUG ONLY
        // if(key_state('m')){
        //     clear_key_state('m');
        //     if(stop){
        //         stop = false;
        //
        //     }
        //     else stop= true;
        // }

        if(key_state('s')){
            // Commencer la partie
            clear_key_state('s');
            startGame = true;
            ticks = SDL_GetTicks();
        }


        if ((unsigned)pipe_parts_number<pipeTEST.pipe_points.size()-1 && !stop) {
            // Faire evoluer notre tuyau de diff cylindres
            diff = std::max(2,std::min(5,(int)(SDL_GetTicks()-ticks)/3));
            pipeTEST.pipe.release();
            pipeTEST.pipe = pipeTEST.progressive_pipe(diff);
            ticks = SDL_GetTicks();
        }

        // Calcul de view et projection
        Transform model= Identity();
        Transform projection = Perspective(45, window_width()/window_height(), 0.1,50.0);
        Transform view;

        Point from = pipeTEST.pipe_points[spaceshipPlacement-10][activeRotation] + (1.5+lost_displacement) * Vector(pipeTEST.pipe_points[spaceshipPlacement-10][activeRotation]- pipeTEST.curve_points[spaceshipPlacement-10]);
        Point to = pipeTEST.pipe_points[spaceshipPlacement-10][activeRotation] + 5* Vector(pipeTEST.curve_points[spaceshipPlacement] - pipeTEST.curve_points[spaceshipPlacement-10]);


        if (!startGame){
            starterCamera.rotation(0.1,0.2);
            view= starterCamera.view();
        }else{
            view = Lookat( from, to, Vector(pipeTEST.pipe_points[spaceshipPlacement-10][activeRotation]- pipeTEST.curve_points[spaceshipPlacement-10]));
        }

        {
            // Affichage de la cubemap
            glBindVertexArray(vao_null);
            glUseProgram(cubemapShader);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            GLint location= glGetUniformLocation(cubemapShader, "texture0");
            glUniform1i(location, 0);
            program_uniform(cubemapShader, "vpInvMatrix", Inverse(projection * view));
            program_uniform(cubemapShader, "camera_position", Inverse(view)(Point(0, 0, 0)));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        // nettoyage
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glBindVertexArray(0);
        glUseProgram(0);

        if (!startGame){
            // Affichage de l'ecran de start
            glUseProgram(i2DShader);
            glBindTexture(GL_TEXTURE_2D, start);
            draw(mainRECT,i2DShader);
            return 1;
        }

        /********************************************************************/
        /*                  Nettoyage des elements dépassés                 */
        /********************************************************************/

        float mydot = dot(normalize(Vector(to,from)),normalize(Vector(Point(cones.getModel(0)[3]),from)));

        while (mydot <0 ){
            //obstacle dépassé
            // on enleve ceux en trop, en theorie: une seule boucle
            cones.popModel();
            mydot = dot(normalize(Vector(to,from)),normalize(Vector(Point(cones.getModel(0)[3]),from)));
        }

        if(checkpointModels.size()>2){
            mydot = dot(normalize(Vector(to,from)),normalize(Vector(Point(checkpointModels[0][3]),from)));
            if (mydot <0 ){
                // checkpoint dépassé
                checkpointModels.erase(checkpointModels.begin());
                // indice du checkpoint à vérifier pour les collisions
                indiceCP--;
            }
        }

        /********************************************************************/
        /*              Initialisation des variables du shader              */
        /********************************************************************/

        glUseProgram(mainShader);

        Transform mv= view * model;
        Transform mvp= projection * mv;

        program_uniform(mainShader, "mvpMatrix", mvp);
        program_uniform(mainShader, "modelMatrix", model);
        program_uniform(mainShader, "usecubemap",0); // utilisation de la reflexion et refraction cubemap
        program_uniform(mainShader, "difuselightstrength", 10);
        program_uniform(mainShader, "viewMatrix", view);
        program_uniform(mainShader, "viewInvMatrix", view.inverse());
        program_uniform(mainShader, "lightPosition", pipeTEST.pipe_points[spaceshipPlacement-5][activeRotation] + 10 * Vector(pipeTEST.pipe_points[spaceshipPlacement-5][activeRotation]-pipeTEST.curve_points[spaceshipPlacement-5]));
        program_uniform(mainShader, "mesh_color", vec4(1,0.7,0.2,1)); // pour les checkpoint
        program_uniform(mainShader, "camera_position", Inverse(view)(Point(0, 0, 0)));

        // texture
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

        /********************************************************************/
        /*                       Affichage du tuyau                         */
        /********************************************************************/
        draw(pipeTEST.pipe,mainShader);


        /********************************************************************/
        /*                    MAJ et Affichage du vaisseau                  */
        /********************************************************************/

        for(int b=spaceshipPlacement; b<spaceshipPlacement+diff; b++){
            spaceshipModel = pipeTEST.curve_rotations[b](spaceshipModel);
        }


        Vector aa = Vector(pipeTEST.curve_points[spaceshipPlacement+1]-pipeTEST.curve_points[spaceshipPlacement]);
        Transform  spaceshipModel2 = Rotation(aa,activeRotation*360/parts)(spaceshipModel);
        spaceshipModel2 = Translation(Vector(pipeTEST.pipe_points[spaceshipPlacement][activeRotation]))(spaceshipModel2);

        mv= view * spaceshipModel2;
        mvp= projection * mv;

        program_uniform(mainShader, "modelMatrix", spaceshipModel2);
        program_uniform(mainShader, "mvpMatrix", mvp);
        draw(Spaceship,mainShader);

        /********************************************************************/
        /*                    Verification des collisions                   */
        /********************************************************************/

        if(!lost){
            Point p1,p2;
            Point p3,p4;
            Spaceship.bounds(p1,p2);
            if (cones.collision(p1,p2,spaceshipModel2)){
                lost = true;
            }

            checkpoint.bounds(p3,p4);
            if((unsigned)indiceCP<checkpointModels.size()){
                if (collides(p1, p2, spaceshipModel2, p3,p4, checkpointModels[indiceCP])){
                    score++;
                    indiceCP++;
                }
            }
        }

        /********************************************************************/
        /*                    MAJ et Affichage des torus                    */
        /********************************************************************/

        for(int b=torusPlacement; b<torusPlacement+diff; b++){
            torusModel = pipeTEST.curve_rotations[b](torusModel);
        }
        aa = Vector(pipeTEST.curve_points[torusPlacement+1]-pipeTEST.curve_points[torusPlacement]);
        Transform torusModel2 = Rotation(aa,global_time()/20)(torusModel);
        torusModel2 = Translation(Vector(pipeTEST.curve_points[torusPlacement]))(torusModel2);
        program_uniform(mainShader, "modelMatrix", torusModel2);
        mv= view * torusModel2;
        mvp= projection * mv;
        program_uniform(mainShader, "mvpMatrix", mvp);

        draw(torus_Part1,mainShader);

        torusModel2 = Rotation(aa,-global_time()/20)(torusModel);
        torusModel2 = Translation(Vector(pipeTEST.curve_points[torusPlacement]))(torusModel2);
        program_uniform(mainShader, "modelMatrix", torusModel2);
        mv= view * torusModel2;
        mvp= projection * mv;
        program_uniform(mainShader, "mvpMatrix", mvp);

        draw(torus_Part2,mainShader);

        /********************************************************************/
        /*                   MAJ et Affichage des CheckPoints               */
        /********************************************************************/

        program_uniform(mainShader, "usecubemap", 1);
        for(int i = 0; (unsigned)i<checkpointModels.size(); i++){
            if( dot(Vector(pipeTEST.curve_points[torusPlacement],pipeTEST.curve_points[torusPlacement+1]),Vector(Point(checkpointModels[i][3]),pipeTEST.curve_points[torusPlacement]))>0){
                program_uniform(mainShader, "modelMatrix", checkpointModels[i]);
                mv= view * checkpointModels[i];
                mvp= projection * mv;
                program_uniform(mainShader, "mvpMatrix", mvp);
                draw(checkpoint,mainShader);
            }
        }

        /********************************************************************/
        /*                    MAJ et Affichage des obstacles                */
        /********************************************************************/

        program_uniform(mainShader, "usecubemap", 0);
        program_uniform(mainShader, "difuselightstrength", 20);

        for(int i=0; i<cones.modelsCount(); i++){
            if( dot(Vector(pipeTEST.curve_points[torusPlacement],pipeTEST.curve_points[torusPlacement+1]),Vector(Point( cones.getModel(i)[3]),pipeTEST.curve_points[torusPlacement]))>0){

                Transform mv= view * cones.getModel(i);
                Transform mvp= projection * mv;
                program_uniform(mainShader, "modelMatrix", cones.getModel(i));
                program_uniform(mainShader, "mvpMatrix", mvp);
                draw(cones.getMesh(), mainShader);
            }
        }

        /********************************************************************/
        /*                    Verif de l'etat du tuyau                      */
        /********************************************************************/

        if ((unsigned)pipe_parts_number > pipeTEST.pipe_points.size()-100){
            // On regenere un segment de tuyau
            int nbpoint = pipeTEST.curve_points.size();
            pipeTEST.genPipe();
            Transform ObstacleInitialRotation = getRotationBetweenVectors(Vector(0,1,0),pipeTEST.getVectorTmp());
            // On y ajoute des obstacles
            addobstacle(std::min(20,10+score),nbpoint-2,ObstacleInitialRotation);

        }

        /********************************************************************/
        /*                         Affichages 2D                            */
        /********************************************************************/

        glUseProgram(i2DShader);

        if (SDL_GetTicks()<(unsigned)ticks+3500 && stop){
            if (SDL_GetTicks()>(unsigned)ticks && SDL_GetTicks()<=(unsigned)ticks+1000){
                glBindTexture(GL_TEXTURE_2D, countDown[2]);
                draw(mainRECT,i2DShader);
            }
            else if (SDL_GetTicks()>(unsigned)ticks+1000 && SDL_GetTicks()<=(unsigned)ticks+2000){
                glBindTexture(GL_TEXTURE_2D, countDown[1]);
                draw(mainRECT,i2DShader);
            }
            else if (SDL_GetTicks()>(unsigned)ticks+2000 && SDL_GetTicks()<=(unsigned)ticks+3000){
                glBindTexture(GL_TEXTURE_2D, countDown[0]);
                draw(mainRECT,i2DShader);
            }
            else {
                stop = false;
            }
        }
        if (lost){
            // affichage du score actif
            lost_displacement=std::min(50.,lost_displacement+0.1);
            glBindTexture(GL_TEXTURE_2D, gameover);
            draw(mainRECT,i2DShader);
            glBindTexture(GL_TEXTURE_2D, numbers[score/10]);
            draw(finalScoreRECT2,i2DShader);
            glBindTexture(GL_TEXTURE_2D, numbers[score%10]);
            draw(finalScoreRECT21,i2DShader);
        }else{
            // affichage du score final
            glBindTexture(GL_TEXTURE_2D, numbers[score%10]);
            draw(activeScoreRECT1,i2DShader);
            glBindTexture(GL_TEXTURE_2D, numbers[score/10]);
            draw(activeScoreRECT2,i2DShader);
        }


        return 1;
    }
    int quit(){
        mainRECT.release();
        pipeTEST.pipe.release();
        // release texture & mesh
        return 0;
    }
    void addobstacle(int n, int indrota, Transform ObstacleInitialRotation){
        int indiceObstacle=indrota;
        Point pmin,pmax;

        Transform cp = RotationZ(90);
        cp = Scale(0.1,0.1,0.1)(cp);

        // Ajout du checkpoint du segment

        if( indrota != 0){
            Transform rotaCP = getRotationBetweenVectors(Vector(1,0,0),normalize(Vector(pipeTEST.curve_points[indrota]-pipeTEST.curve_points[indrota-1])));
            cp = rotaCP(cp);
            cp = Translation(Vector(pipeTEST.curve_points[indiceObstacle]))(cp);
        }
        else{            Transform rotaCP = getRotationBetweenVectors(Vector(1,0,0),normalize(Vector(pipeTEST.curve_points[1]-pipeTEST.curve_points[0])));
            cp = rotaCP(cp);
            cp = Translation(Vector(pipeTEST.curve_points[spaceshipPlacement+5]))(cp);
        }
        checkpointModels.push_back(cp);

        // Ajout des obstacles
        cones.getBounds(pmin,pmax);
        for(int i =0; i<n; i++){
            Transform model2;
            float rdx = rand()%10;
            rdx = rdx / 1000;
            float rdy = rand()%20;
            rdy = rdy / 1000;
            float rdz= rand()%10;
            rdz = rdz / 1000;

            // pour pas qu'il flotte

            model2 = Translation(Vector(0,3*(pmax.y-pmin.y)/2/4,0));
            model2 = Scale(0.01+rdx,0.01+rdy,0.01+rdz)(model2);

            model2 = ObstacleInitialRotation(model2);
            int test =  (pipeTEST.pipe_points.size() - indrota)/n;
            int posmin = rand()%3 -1;
            indiceObstacle = indiceObstacle + test + 0.1 * test * posmin;
            if ((unsigned)indiceObstacle > pipeTEST.pipe_points.size()-1)
            return;

            for(int b=indrota; b<indiceObstacle; b++){
                model2 = pipeTEST.curve_rotations[b](model2);
            }


            int rda = rand()%parts;
            Vector a = Vector(pipeTEST.curve_points[indiceObstacle+1]-pipeTEST.curve_points[indiceObstacle]);
            model2 = Rotation(a,rda*360/parts)(model2);
            model2 = Translation(Vector(pipeTEST.pipe_points[indiceObstacle][rda]))(model2);
            cones.addModel(model2);

        }


    }
    int init(){

        /********************************************************************/
        /*                 Initialisation des Mesh 2d                       */
        /********************************************************************/

        mainRECT.texcoord(vec2(0,0)).vertex(-1,-1,0);
        mainRECT.texcoord(vec2(1,0)).vertex(1,-1,0);
        mainRECT.texcoord(vec2(0,1)).vertex(-1,1,0);

        mainRECT.texcoord(vec2(0,1)).vertex(-1,1,0);
        mainRECT.texcoord(vec2(1,0)).vertex(1,-1,0);
        mainRECT.texcoord(vec2(1,1)).vertex(1,1,0);

        activeScoreRECT1.texcoord(vec2(0,0)).vertex(0.875,0.75,0);
        activeScoreRECT1.texcoord(vec2(1,0)).vertex(1,0.75,0);
        activeScoreRECT1.texcoord(vec2(0,1)).vertex(0.875,1,0);

        activeScoreRECT1.texcoord(vec2(0,1)).vertex(0.875,1,0);
        activeScoreRECT1.texcoord(vec2(1,0)).vertex(1,0.75,0);
        activeScoreRECT1.texcoord(vec2(1,1)).vertex(1,1,0);

        activeScoreRECT2.texcoord(vec2(0,0)).vertex(0.825,0.75,0);
        activeScoreRECT2.texcoord(vec2(1,0)).vertex(0.950,0.75,0);
        activeScoreRECT2.texcoord(vec2(0,1)).vertex(0.825,1,0);

        activeScoreRECT2.texcoord(vec2(0,1)).vertex(0.825,1,0);
        activeScoreRECT2.texcoord(vec2(1,0)).vertex(0.950,0.75,0);
        activeScoreRECT2.texcoord(vec2(1,1)).vertex(0.950,1,0);

        finalScoreRECT2.texcoord(vec2(0,0)).vertex(0.5,-0.31,0);
        finalScoreRECT2.texcoord(vec2(1,0)).vertex(0.625,-0.31,0);
        finalScoreRECT2.texcoord(vec2(0,1)).vertex(0.5,0,0);

        finalScoreRECT2.texcoord(vec2(0,1)).vertex(0.5,0,0);
        finalScoreRECT2.texcoord(vec2(1,0)).vertex(0.625,-0.31,0);
        finalScoreRECT2.texcoord(vec2(1,1)).vertex(0.625,0,0);

        finalScoreRECT21.texcoord(vec2(0,0)).vertex(0.55,-0.31,0);
        finalScoreRECT21.texcoord(vec2(1,0)).vertex(0.675,-0.31,0);
        finalScoreRECT21.texcoord(vec2(0,1)).vertex(0.55,0,0);

        finalScoreRECT21.texcoord(vec2(0,1)).vertex(0.55,0,0);
        finalScoreRECT21.texcoord(vec2(1,0)).vertex(0.675,-0.31,0);
        finalScoreRECT21.texcoord(vec2(1,1)).vertex(0.675,0,0);

        /********************************************************************/
        /*            Initialisation des shaders obj et images              */
        /********************************************************************/

        cubemapShader= read_program("data/shaders/cubemap.glsl");
        program_print_errors(cubemapShader);

        i2DShader= read_program("data/shaders/2d.glsl");
        program_print_errors(i2DShader);

        mainShader= read_program("data/shaders/pipe.glsl");
        program_print_errors(mainShader);

        cones = Obstacle("data/obj/cone_corrige.obj");
        torus_Part1 = read_mesh("data/obj/torus1.obj"); // data/cone.obj
        torus_Part2 = read_mesh("data/obj/torus2.obj"); // data/cone.obj
        checkpoint = read_mesh("data/obj/checkpoint3.obj"); // data/cone.obj
        Spaceship = read_mesh("data/obj/VAISS.obj"); // data/cone.obj

        ImageData cubemapImage= read_image_data("data/cubemap/cubemap5.png");

        countDown[2]= read_texture(0, "data/textures/3.png");
        countDown[1]= read_texture(0, "data/textures/2.png");
        countDown[0]= read_texture(0, "data/textures/1.png");

        numbers[0] = read_texture(0, "data/textures/0.png");
        numbers[1] = read_texture(0, "data/textures/1p.png");
        numbers[2] = read_texture(0, "data/textures/2p.png");
        numbers[3] = read_texture(0, "data/textures/3p.png");
        numbers[4] = read_texture(0, "data/textures/4p.png");
        numbers[5] = read_texture(0, "data/textures/5p.png");
        numbers[6] = read_texture(0, "data/textures/6p.png");
        numbers[7] = read_texture(0, "data/textures/7p.png");
        numbers[8] = read_texture(0, "data/textures/8p.png");
        numbers[9] = read_texture(0, "data/textures/9p.png");

        gameover = read_texture(0, "data/textures/gameover.png");
        start = read_texture(0, "data/textures/start2.png");

        /********************************************************************/
        /*                   Initialisation des models                      */
        /********************************************************************/

        // Ajout des obstacles
        Transform ObstacleInitialRotation = getRotationBetweenVectors(Vector(0,1,0),normalize(pipeTEST.getVectorTmp()));
        addobstacle(std::min(20,10+score),0,ObstacleInitialRotation);

        // Model du vaisseau
        Point pmin,pmax;
        Spaceship.bounds(pmin,pmax);
        spaceshipModel = RotationY(-180);
        spaceshipModel = Translation(Vector(0,1.2*(pmax.y-pmin.y),0))(spaceshipModel);
        spaceshipModel = Scale(0.015,0.015,0.015)(spaceshipModel);
        spaceshipModel = ObstacleInitialRotation(spaceshipModel);
        for(int b=0; b<spaceshipPlacement; b++){
            spaceshipModel = pipeTEST.curve_rotations[b](spaceshipModel);
        }

        // Model des torus
        torusModel = RotationX(90);
        torusModel = Scale(0.1,0.1,0.1)(torusModel);
        torusModel = ObstacleInitialRotation(torusModel);
        for(int b=0; b<torusPlacement; b++){
            torusModel = pipeTEST.curve_rotations[b](torusModel);
        }

        /********************************************************************/
        /*                 Initialisation de la cubemap                     */
        /********************************************************************/
        // code du tuto adapté
        int size= cubemapImage.width / 4;
        int height = cubemapImage.height / 3;

        GLenum data_format;
        GLenum data_type= GL_UNSIGNED_BYTE;
        if(cubemapImage.channels == 3)
        data_format= GL_RGB;
        else // par defaut
        data_format= GL_RGBA;

        glGenTextures(1, &cubemapTexture);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

        glPixelStorei(GL_UNPACK_ROW_LENGTH, cubemapImage.width);

        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 2*size + 1*height*cubemapImage.width);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0,GL_RGBA, size, size, 0,data_format, data_type, cubemapImage.buffer());

        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0*size + 1*height*cubemapImage.width);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0,GL_RGBA, size, size, 0,data_format, data_type, cubemapImage.buffer());

        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 1*size + 0*height*cubemapImage.width);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0,GL_RGBA, size, size, 0,data_format, data_type, cubemapImage.buffer());

        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 1*size + 2*height*cubemapImage.width);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0,GL_RGBA, size, size, 0,data_format, data_type, cubemapImage.buffer());

        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 1*size +1*height*cubemapImage.width);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0,GL_RGBA, size, size, 0,data_format, data_type, cubemapImage.buffer());

        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 3*size+1*height*cubemapImage.width);
        glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0,GL_RGBA, size, size, 0,data_format, data_type, cubemapImage.buffer());

        glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
        // filtrage "correct" sur les bords du cube...
        glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

        // nettoyage
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);

        // etape 4 : vao pour dessiner la cubemap a l'infini
        glGenVertexArrays(1, &vao_null);
        glBindVertexArray(vao_null);
        // pas de buffer, c'est le vertex shader qui genere directement les positions des sommets

        glUseProgram(0);
        glBindVertexArray(0);

        // etat par defaut
        glClearColor(0.2f, 0.2f, 0.2f, 1);
        glClearDepthf(1);

        glDepthFunc(GL_LEQUAL);     // !! attention !! le support de la cube map est dessine exactement sur le plan far
        glFrontFace(GL_CCW);
        glCullFace(GL_BACK);
        glEnable(GL_CULL_FACE);

        glClearColor(0.2f, 0.18f, 0.25f, 1.f); // couleur par defaut de la fenetre
        //Gestion face cachée et affichage 3D
        glClearDepth(1.f); // profondeur par defaut
        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_BACK);// doesnt render hidden parts
        glEnable(GL_CULL_FACE);
        // optimizing
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        return 0;
    }



private:
    // Main pipe
    Pipe pipeTEST;

    // 2D MESH
    Mesh mainRECT;
    Mesh activeScoreRECT1;
    Mesh activeScoreRECT2;
    Mesh finalScoreRECT2;
    Mesh finalScoreRECT21;
    // 3D MESH
    Mesh torus_Part1;
    Mesh torus_Part2;
    Mesh checkpoint;
    Mesh Spaceship;

    Orbiter starterCamera;

    // Obstacles
    Obstacle cones;

    // Shaders
    GLuint mainShader;
    GLuint cubemapShader;
    GLuint i2DShader;

    // Textures
    ImageData cubemapImage;
    GLuint cubemapTexture;
    GLuint countDown[3];
    GLuint start;
    GLuint gameover;
    GLuint vao_null;
    GLuint numbers[10];

    // Models
    Transform spaceshipModel;
    Transform torusModel;
    std::vector<Transform> checkpointModels;

    int activeRotation; // active part placement on pipe
    bool stop; // is the progression stopped
    int torusPlacement; // torus point number placement
    int spaceshipPlacement; // spaceship point number placement
    int ticks; // used for countdown and diff
    int parts; // number of cylinder parts
    int pipe_parts_number; // number of cylinder displayed
    bool startGame; // is the game started
    bool lost; // did we lost
    float lost_displacement;
    int indiceCP; // CP index to check
    int score; // actual score
};


int main(int argc, char** argv){

    Project mt(1024, 640);
    mt.run();
    return 0;
}
