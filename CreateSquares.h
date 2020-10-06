
#ifndef CreateSquares_h
#define CreateSquares_h
using namespace ci;
using namespace ci::app;
using namespace std;

class CreateSquares {
    int nSquare;

public:
    CreateSquares() {
       int nSquare = 4;
    }

    void draw(cv::Mat )
    {
        
        int squareW = app::getWindowWidth() / nSquare;
        int squareH = app::getWindowHeight() / nSquare;

        //creating squares
        for (int i = 0; i < nSquare; i++) {
            for (int j = 0; j < nSquare; j++) {
                int x1 = i * squareW; 
                int y1 = j * squareH;
                int x2 = x1 + squareW;             
                int y2 = y1 + squareH;

                Rectf curSquare = Rectf(x1, y1, x2, y2);
                int sum = 0;
                

               


                gl::drawSolidRect(curSquare);
            }
        }



    }
  
};


#endif // Squares_h