#pragma once
#include<math.h>
#include<iostream>
#include <vector>

#define PI 3.14159265
using namespace std;

struct position {
	// position of the uav
	float x;
	float y;
	float z;
};

struct velocity {
	// velocity of the uav
	float vx;
	float vy;
	float vz;
};
 
struct acceleration {
	// acceleration of the uav
	float ax;
	float ay;
	float az;
};

struct generalforce {
	float fx;
	float fy;
	float fz;
};

class uav {
private:
	// define basic state of the uav
	position pos = { 0.0, 0.0, 0.1 };
	velocity vel = { 0.0, 0.0, 0.0 };
	acceleration acc = { 0.0, 0.0, 0.0 };

	// system parameters
	float mass = 3.0; // mass, kg

	// internal and external forces
	generalforce F_motor = { 0.0, 0.0, 0.0 };	// force provided by motors
	generalforce F_drag = { 0.0, 0.0, 0.0 };	// froce by air drag
	generalforce F_coulomb = { 0.0, 0.0, 0.0 }; // Repulsive coulomb force
	generalforce F_join = { 0.0, 0.0, 0.0 };	// join force

public:
	// basic controls
	void forward();
	void backward();
	void left();
	void right();
	void up();
	void down();
	void yawleft();
	void yawright();

	// calculate drone's current join force and accerelation
	void dynamics();
};