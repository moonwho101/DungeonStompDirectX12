#ifndef CAMERABOB_H
#define CAMERABOB_H

class CameraBob {

private:
    float x = 0;
    float y = 0;
    float s = 0;
    float a = 0;
    float f = 0;
    bool centering = false;
    float centerSpeed = 8.0f;

public:

    CameraBob() {

    }

    void SinWave(float speed, float amplitude, float frequency);
    void update(float delta);
    float CameraBob::getX();

    float CameraBob::getY();
    float CameraBob::getSpeed();
    float CameraBob::getAmplitude();
    float CameraBob::getFrequency();
    void CameraBob::setX(float x2);
    void CameraBob::setY(float y1);
    void CameraBob::setSpeed(float speed);
    void CameraBob::setAmplitude(float amplitude);
    void CameraBob::setFrequency(float frequency);
    void startCentering(float speed = 8.0f);
    void stopCentering();
};

#endif // CAMERABOB_H



