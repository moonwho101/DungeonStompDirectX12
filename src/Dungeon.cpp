#include "Dungeon.h"
#include <ppl.h>
#include <algorithm>
#include <vector>
#include <cassert>

using namespace DirectX;

Dungeon::Dungeon(int m, int n, float dx, float dt, float speed, float damping) {
	mNumRows = m;
	mNumCols = n;

	mVertexCount = m * n;
	mTriangleCount = (m - 1) * (n - 1) * 2;

	mTimeStep = dt;
	mSpatialStep = dx;
}

Dungeon::~Dungeon() {
}

int Dungeon::RowCount() const {
	return mNumRows;
}

int Dungeon::ColumnCount() const {
	return mNumCols;
}

int Dungeon::VertexCount() const {
	return mVertexCount;
}

int Dungeon::TriangleCount() const {
	return mTriangleCount;
}

float Dungeon::Width() const {
	return mNumCols * mSpatialStep;
}

float Dungeon::Depth() const {
	return mNumRows * mSpatialStep;
}

void Dungeon::Update(float dt) {
}

void Dungeon::Disturb(int i, int j, float magnitude) {
}
