
using namespace cinder;
using namespace ci::app;
using namespace std;

#ifndef ExampleSquares_h
#define ExampleSquares_h

class ExampleSquares
{
protected:
    int N;
    virtual float count(ci::Rectf)=0;
    virtual float getDivisorOfSum()=0;
   

    int width = getWindowWidth() / N;
    int height = getWindowHeight() / N;
public:
   virtual void setN(int n) {
        N = n; 
    }
    
    virtual void drawRect()
    {
        int Width = 0;
        int Height = 0;

        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                int x1 = i * Width;  
                int y1 = j * Height;
                int x2 = x1 + Width;  
                int y2 = y1 + Height;
                Rectf curSquare = Rectf(x1, y1, x2, y2); ;  
                float sum = count(curSquare);
                gl::color(sum / getDivisorOfSum(), 0, 0, 1); 
                gl::drawSolidRect(curSquare);   
            }
        }


    }

};



class SquaresFrameDiff : public ExampleSquares
{
private:
    cv::Mat frameDiff;
public:
    virtual void drawRect(cv::Mat b)
    {
        frameDiff = b;
        ExampleSquares::drawRect();
        
    }
    virtual float count(ci::Rectf curSquare)
    {
        int sum = 0;
        for (int y = curSquare.getY1(); y < curSquare.getY2(); y++) {
            for (int x = curSquare.getX1(); x < curSquare.getX2(); x++) {
                int pixels = frameDiff.at<uint8_t>(y, x);
                sum +=pixels;
            }
        }
        return sum;
    }
    
    float getDivisorOfSum()
    {
        return 10; 
    }
    

};

class SquaresFeatures : public ExampleSquares
{
    private:
    std::vector<cv::Point2f> features;
    
  public:
    virtual void drawRect(std::vector<cv::Point2f> pts)
    {
        features = pts;
        ExampleSquares::drawRect();
        
    }

    virtual float count(ci::Rectf curSquare)
    {
        int Sum = 0;
        for (int i = 0; i < features.size(); i++) {
            if (curSquare.contains(fromOcv(features[i]))) {
                Sum++;


            }

        }
        return Sum;
    }
    
    float getDivisorOfSum()
    {
        return 10;//match to the max num of feature found 
    }
    
};





#endif /* Header_h */
