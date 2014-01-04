//
//  CloudsTransitionController.h
//  CLOUDS
//
//  Created by James George on 1/3/14.
//
//

#pragma once

#include "ofMain.h"

typedef enum {
	TRANSITION_IDLE = 0,
	TRANSITION_INTERVIEW_OUT = 1,
	TRANSITION_VISUALSYSTEM_IN = 2,
	TRANSITION_VISUALSYSTEM_OUT = 3,
	TRANSITION_INTERVIEW_IN = 4,
	TRANSITION_INTRO_OUT = 5
} CloudsTransitionState;

//NOT USED YET
typedef struct {
	float startTime;
	float endTime;
	CloudsTransitionState state;
} CloudsTransitionQueueEntry;

class CloudsTransitionController {
  public:
	CloudsTransitionController();
	
	void transitionFromIntro(float transitionOutDuration, float transitionInDuration);
	void transitionToVisualSystem(float transitionOutDuration, float transitionInDuration);
	void transitionToInterview(float transitionOutDuration, float transitionInDuration);
	void transitionToClusterMap(float transitionOutDuration, float transitionInDuration);
	
	void update();
	
	float transitionPercent;
//	float percentTransitionIn;
//	float percentTransitionOut;

	float getInterviewTransitionPoint();
	
	bool transitioning;
	bool triggeredMidpoint;
	bool fadingOut();
	
	//returns 0 - 1.0 for use in alpha on the visual system texture
	//ramps down when fading out, then up when fading in
	float getFadeValue();
	
	bool isStateNew();
	CloudsTransitionState getCurrentState();
	CloudsTransitionState getPreviousState();
	string getCurrentStateDescription();
	
  protected:
	
	deque<CloudsTransitionQueueEntry> stateQueue;
	void queueState(CloudsTransitionState state, float transitionDuration);
	CloudsTransitionQueueEntry currentQueue;
//	void startTransition(float transitionOutDuration, float transitionInDuration);
	void startTransition();
	
	CloudsTransitionState currentState;
	CloudsTransitionState previousState;
	float transitionStartTime;
	
	float transitionInCompleteTime;
	float transitionOutCompleteTime;
	void confirmEmpty();
	
	bool newState;
	CloudsTransitionState getNextState();
	
};
