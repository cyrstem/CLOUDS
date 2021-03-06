//
//  Maze.cpp
//  MazeGenerator
//
//  Created by Gal Sasson on 10/25/13.
//
//

#include "Maze2.h"

//#define SIDE_WALLS

Maze2::Maze2(float cSize, float wThickness, float wHeight, ofVec3f p)
{
    cellSize = cSize;
    wallThickness = wThickness;
    wallHeight = wHeight;
    pos = p;
//    cells = new Maze2[NUM_CELLS_X2] [NUM_CELLS_Y2];
    cells.resize(NUM_CELLS_X2);
    for (int i=0; i<NUM_CELLS_X2; i++) {
        cells[i].resize(NUM_CELLS_Y2);
        for (int j=0; j<NUM_CELLS_Y2; j++)
        {
            cells[i][j] = new MazeCell2(i, j, cellSize, wallThickness, wallHeight);
        }
    }
    
    currentCell = NULL;
    exitCell = NULL;
    
    finishedGenerating = false;
    finishedSolving = false;
    step = 0;
    currentYLimit = 40;
}

Maze2::~Maze2()
{
    for (int i=0; i<NUM_CELLS_X2; i++)
    {
        for (int j=0; j<NUM_CELLS_Y2; j++)
        {
            delete cells[i][j];
        }
    }
}

void Maze2::generate()
{
    // set starting (exit) point
    int randX = 0;
    int randY = 0;
    currentCell = cells[randX][randY];
    currentCell->visit();
    currentCell->mazeExit = true;
    cellStack.push(currentCell);
    exitCell = currentCell;
    finishedGenerating = false;
    
    // generate the maze
//    while (!finishedGenerating) {
//        generateStep();
//    }
    for (int i=0; i<500; i++)
    {
        generateStep();
    }
}

void Maze2::update(ofCamera *cam)
{
    if (!finishedGenerating) {
        for (int i=0; i<4; i++) {
            generateStep();
        }
    }
}

void Maze2::draw(ofCamera *cam)
{
    ofPushMatrix();
    ofTranslate(pos);
    // for tiling
    int yStart = cam->getPosition().z/cellSize-25;
    int yLimit = min(yStart+(int)ParamManager2::getInstance().showAhead,NUM_CELLS_Y2);
    if (yStart < 0) {
        yStart = 0;
    }
    float length = (float)yLimit - yStart;
    float middle = (float)yStart + length/2;

    // draw the ground
    ofFill();
    ofSetColor(ParamManager2::getInstance().getGroundColor());
    ofPushMatrix();
    ofTranslate(NUM_CELLS_X2*cellSize/2, -wallHeight/2, middle*cellSize);
    ofScale(NUM_CELLS_X2*cellSize+120, 1, length*cellSize);
    ofBox(1);
    ofPopMatrix();
    
    // draw side walls
#ifdef SIDE_WALLS
    ofSetColor(ParamManager::getInstance().getSideWallsColor());
    ofPushMatrix();
    ofTranslate(-0.1, 800-wallHeight/2, middle*cellSize);
    ofScale(wallThickness, 1600, length*cellSize);
    ofBox(1);
    ofPopMatrix();
    ofPushMatrix();
    ofTranslate(NUM_CELLS_X2*cellSize+0.1, 800-wallHeight/2, middle*cellSize);
    ofScale(wallThickness, 1600, length*cellSize);
    ofBox(1);
    ofPopMatrix();
#endif
    
    // draw the cells
    for (int i=0; i<NUM_CELLS_X2; i++)
    {
        for (int j=yStart; j<yLimit; j++)
        {
            cells[i][j]->draw(currentCell == cells[i][j]);
        }
    }
    
    ofPopMatrix();
}

float Maze2::getWidth()
{
    return NUM_CELLS_X2*cellSize;
}

void Maze2::generateStep()
{
    if (finishedGenerating) { return; }
    bool valid = false;
    std::vector<int> available_dirs;
    for (int i=0;i<4;i++)
    {
        available_dirs.push_back(i);
        
        
    }
    while (!valid) {
        int curx = currentCell->getX();
        int cury = currentCell->getY();
        int dir_idx = int(ofRandom(available_dirs.size()));
        int dir = available_dirs[dir_idx];
        
        switch(dir) {
            case 0:
                if (cury > 0 && cells[curx][cury-1]->notVisited()) {
                    currentCell->top = false;
                    currentCell = cells[curx][cury-1];
                    currentCell->visit();
                    currentCell->bottom = false;
                    cellStack.push(currentCell);
                    valid = true;
                } else {
                    available_dirs.erase(available_dirs.begin() + dir_idx);
                }
                break;
            case 1:
                if (curx < NUM_CELLS_X2-1 && cells[curx+1][cury]->notVisited()) {
                    currentCell->right = false;
                    currentCell = cells[curx+1][cury];
                    currentCell->visit();
                    currentCell->left = false;
                    cellStack.push(currentCell);
                    valid = true;
                } else {
                    available_dirs.erase(available_dirs.begin() + dir_idx);
                }
                break;
            case 2:
                if (cury < min(NUM_CELLS_Y2-1,currentYLimit-1) && cells[curx][cury+1]->notVisited()) {
                    currentCell->bottom = false;
                    currentCell = cells[curx][cury+1];
                    currentCell->visit();
                    currentCell->top = false;
                    cellStack.push(currentCell);
                    valid = true;
                } else {
                    available_dirs.erase(available_dirs.begin() + dir_idx);
                }
                break;
            case 3:
                if (curx > 0 && cells[curx-1][cury]->notVisited()) {
                    currentCell->left = false;
                    currentCell = cells[curx-1][cury];
                    currentCell->visit();
                    currentCell->right = false;
                    cellStack.push(currentCell);
                    valid = true;
                } else {
                    available_dirs.erase(available_dirs.begin() + dir_idx);
                }
                break;
        }  // end switch
        if (!valid && available_dirs.size() == 0) {
            if (!cellStack.empty()) {
                currentCell = cellStack.top();
                cellStack.pop();
            }
            else {
                int prevLimit = currentYLimit;
                currentYLimit += 60;
                if (prevLimit > NUM_CELLS_Y2) {
                    finishedGenerating = true;
                    currentCell = NULL;
                }
                else {
                    currentCell = cells[0][prevLimit+1];
                }
            }
            valid = true;
        }
    } // end while
}


