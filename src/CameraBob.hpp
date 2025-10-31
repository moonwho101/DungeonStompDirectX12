#ifndef CAMERABOB_H
#define CAMERABOB_H

class CameraBob {

  private:
	float x = 0;
	float y = 0;
	float s = 0;
	float a = 0;
	float f = 0;
	bool isBobbing = false; // Added state variable

  public:
	CameraBob() {
		isBobbing = false; // Initialize isBobbing
	}

	void SinWave(float speed, float amplitude, float frequency);
	void update(float delta);
	void stopBobbing();        // Added declaration for stopBobbing
	bool getIsBobbing() const; // Getter for isBobbing
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
};

#endif // CAMERABOB_H
