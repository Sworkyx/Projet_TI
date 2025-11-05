#include "camera.h"
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <opencv2/opencv.hpp>

using namespace cv;
using namespace std;

Camera::Camera()
{
    m_fps = 30;
}

bool Camera::open(std::string filename)
{
    m_fileName = filename;

    // Convert filename to number if you want to open webcam stream
    std::istringstream iss(filename.c_str());
    int devid;
    bool isOpen = false;

    if (!(iss >> devid))
    {
        // Fichier vidéo
        isOpen = m_cap.open(filename);
    }
    else
    {
        // Webcam (ex: "0")
        isOpen = m_cap.open(devid, cv::CAP_ANY);
    }

    if (!isOpen)
    {
        std::cerr << "Unable to open video file or camera." << std::endl;
        return false;
    }

    // Lire le framerate, sinon mettre 30 par défaut
    m_fps = m_cap.get(cv::CAP_PROP_FPS);
    if (m_fps == 0)
        m_fps = 30;

    return true;
}

void Camera::play()
{
    // Créer la fenêtre
    cv::namedWindow("Video", cv::WINDOW_AUTOSIZE);
    bool isReading = true;

    // Calcul du délai entre les frames
    int timeToWait = static_cast<int>(1000 / m_fps);

	// Compteur de frames
	int frameCounter = 0;

	cv::Mat hsv,gray;
    cv::Mat mask_b;
    cv::Mat img_sans_v,img_v;
    cv::Mat test;
    cv::Mat voie_gauche,voie_droite;
	std::vector<cv::Vec4i> lines;
    int cpt_droite = 0;
    int cpt_gauche = 0;
    int prev_countnzero_droite = 0;
    int prev_countnzero_gauche = 0;
    Rect rect1(20, 100, 110, 2);
    Rect rect2(550, 100, 230, 2);

    int largeur = m_cap.get(cv::CAP_PROP_FRAME_WIDTH);
    printf("Largeur: %d\n",largeur);
    int hauteur = m_cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    printf("Hauteur: %d\n",hauteur);

    while (isReading)
    {
        // Lire une frame
        isReading = m_cap.read(m_frame);

        if (isReading)
        {
			// On prend la 3eme frame pour placer les bordures de la route
			if (frameCounter == 3){
				//on convertit l'image en HSV
				cv::cvtColor(m_frame, hsv, cv::COLOR_BGR2HSV);
				//on applique un mask pour ne garder que le rouge - violet
				inRange(hsv, cv::Scalar(120,0,0), cv::Scalar(180, 255, 255), mask_b);
                //imshow("ihvuy",mask_b);

				//on dilate l'image pour combler les trous puis on erode
				cv::dilate(mask_b, mask_b, cv::Mat(), cv::Point(-1,-1),12);
				cv::erode(mask_b, mask_b, cv::Mat(), cv::Point(-1,-1),12);
				//on applique un filtre de Canny pour detecter les contours
                cv::Canny(mask_b, mask_b, 100, 200, 3);
				//on applique le filtre de huffman pour detecter les lignes droites
				HoughLinesP(mask_b, lines, 1, CV_PI/180, 10, 180, 60);                	
				cv::imshow("mask_b_canny.jpg", mask_b);

                //on choisit une image sans véhicule pour permettre la détection
                m_frame.copyTo(img_sans_v);
                
			}
			// On dessine les lignes détectées à partir de la 4eme frame
			if(frameCounter > 3)
			{
                absdiff(m_frame,img_sans_v,img_v);
				cv::cvtColor(img_v, img_v, cv::COLOR_BGR2GRAY);
				inRange(img_v, cv::Scalar(60,60,60), cv::Scalar(255, 255, 255), img_v);

                //on dilate l'image pour combler les trous puis on erode
				cv::dilate(img_v, img_v, cv::Mat(), cv::Point(-1,-1),10);
				cv::erode(img_v, img_v, cv::Mat(), cv::Point(-1,-1),10);
                cv::Canny(img_v, img_v, 200, 200, 3);

                //Compter les véhicules détectés dans chaque sens
                //On choisit une portion d'image pour isoler les deux sens 
                cv::rectangle(m_frame, cv::Point(20,100), cv::Point(230,102), cv::Scalar(255,255,255),2);
                cv::rectangle(m_frame,cv::Point(550,100), cv::Point(780,102),cv::Scalar(255,255,255),2);

                voie_gauche = img_v(rect1);
                voie_droite = img_v(rect2);

                if(cv::countNonZero(voie_droite) - prev_countnzero_droite > 10){
                    cpt_droite++;
                    printf("compteur droit = %d \n",cpt_droite);
                }
                if(cv::countNonZero(voie_gauche) - prev_countnzero_gauche > 5){  
                    cpt_gauche++;
                    printf("compteur gauche = %d \n",cpt_gauche);
                }

                prev_countnzero_droite = cv::countNonZero(voie_droite);
                prev_countnzero_gauche = cv::countNonZero(voie_gauche);
            
				for( size_t i = 0; i < lines.size(); i++ )
					{
						cv::line( m_frame, cv::Point(lines[i][0], lines[i][1]), cv::Point(lines[i][2], lines[i][3]), cv::Scalar(0,255,0), 3, cv::LINE_AA);
				}
                
            }

            //on cherche a identifier détecter les véhicules et afficher un carré autour des véhicules détecter

            // Afficher la frame
            cv::imshow("Video", m_frame);

			frameCounter++;
        }
        else
        {
            std::cerr << "Unable to read device" << std::endl;
            break;
        }

        // Quitter si 'Échap' est pressé
        if (cv::waitKey(timeToWait) == 27)
        {
            std::cerr << "Stopped by user" << std::endl;
            isReading = false;
        }
    }
}

bool Camera::close()
{
    // Fermer le flux
    m_cap.release();

    // Fermer les fenêtres
    cv::destroyAllWindows();

    usleep(100000);

    return true; // <-- ajout recommandé
}
