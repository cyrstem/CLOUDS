
#include "CloudsVisualSystem.h"
#include "CloudsRGBDCombinedRenderer.h"

static ofFbo sharedRenderTarget;
static ofImage sharedCursor;

ofFbo& CloudsVisualSystem::getSharedRenderTarget(){
    if(!sharedRenderTarget.isAllocated() ||
       sharedRenderTarget.getWidth() != ofGetWidth() ||
       sharedRenderTarget.getHeight() != ofGetHeight())
    {
        sharedRenderTarget.allocate(ofGetWidth(), ofGetHeight(), GL_RGB, 4);
		sharedRenderTarget.begin();
		ofClear(0,0,0,0);
		sharedRenderTarget.end();
		
    }
    return sharedRenderTarget;
}

ofImage& CloudsVisualSystem::getCursor(){
	if(!sharedCursor.bAllocated()){
		sharedCursor.loadImage(getDataPath() + "images/cursor.png");
	}
	return sharedCursor;
}

CloudsVisualSystem::CloudsVisualSystem(){
	isPlaying = false;
	sharedRenderer = NULL;
	bClearBackground = true;
}

CloudsVisualSystem::~CloudsVisualSystem(){
	
}

string CloudsVisualSystem::getVisualSystemDataPath(){
    return getDataPath() + "visualsystems/"+getSystemName()+"/";
}


void CloudsVisualSystem::setup(){
	
	cout << "SETTING UP SYSTEM " << getSystemName() << endl;
	
	ofAddListener(ofEvents().exit, this, &CloudsVisualSystem::exit);
    
	currentCamera = &cam;
	
    ofDirectory dir;
    string directoryName = getVisualSystemDataPath();
    if(!dir.doesDirectoryExist(directoryName))
    {
        dir.createDirectory(directoryName);
    }
    
    string workingDirectoryName = directoryName+"Working/";
    if(!dir.doesDirectoryExist(workingDirectoryName))
    {
        dir.createDirectory(workingDirectoryName);
    }
    
    setupAppParams();
    setupDebugParams();
    setupCameraParams();
	setupLightingParams();
	setupMaterialParams();
    setupTimeLineParams();
    
    selfSetup();
    setupCoreGuis();
    selfSetupGuis();
	
	setupTimeline();
    setupTimelineGui();
    
	hideGUIS();
	
}

void CloudsVisualSystem::playSystem(){
	if(!isPlaying){
		
		ofRegisterMouseEvents(this);
		ofRegisterKeyEvents(this);
		ofAddListener(ofEvents().update, this, &CloudsVisualSystem::update);
		ofAddListener(ofEvents().draw, this, &CloudsVisualSystem::draw);
		
		isPlaying = true;
		
		loadGUIS();
		timeline->hide();
		hideGUIS();
//		ofHideCursor();
		
		//    showGUIS(); //remove this so the gui doesn't keep popping up
		cam.enableMouseInput();
		for(map<string, ofxLight *>::iterator it = lights.begin(); it != lights.end(); ++it)
		{
			it->second->light.setup();
		}
		
		selfBegin();

	}
}

void CloudsVisualSystem::stopSystem(){
	if(isPlaying){

		selfEnd();
		
		hideGUIS();
		saveGUIS();
		cam.disableMouseInput();
		for(map<string, ofxLight *>::iterator it = lights.begin(); it != lights.end(); ++it)
		{
			it->second->light.destroy();
		}
		
		ofUnregisterMouseEvents(this);
		ofUnregisterKeyEvents(this);
		ofRemoveListener(ofEvents().update, this, &CloudsVisualSystem::update);
		ofRemoveListener(ofEvents().draw, this, &CloudsVisualSystem::draw);
		
		isPlaying = false;
	}
}

float CloudsVisualSystem::getSecondsRemaining(){
	return secondsRemaining;
}

void CloudsVisualSystem::setSecondsRemaining(float seconds){
	secondsRemaining = seconds;
}

void CloudsVisualSystem::setCurrentKeyword(string keyword){
	currentKeyword = keyword;
}

string CloudsVisualSystem::getCurrentKeyword(){
	return currentKeyword;
}

void CloudsVisualSystem::setCurrentTopic(string topic){
	currentTopic = topic;
}

string CloudsVisualSystem::getCurrentTopic(){
	return currentTopic;
}

void CloudsVisualSystem::setRenderer(CloudsRGBDCombinedRenderer& newRenderer){
	sharedRenderer = &newRenderer;
}

void CloudsVisualSystem::setupSpeaker(string speakerFirstName,
									  string speakerLastName,
									  string quoteName)
{
	this->speakerFirstName = speakerFirstName;
	this->speakerLastName = speakerLastName;
	this->quoteName = quoteName;
	hasSpeaker = true;
	
}

void CloudsVisualSystem::speakerEnded(){
	hasSpeaker = false;
}

#define REZANATOR_GUI_ALPHA_MULTIPLIER 4

void CloudsVisualSystem::update(ofEventArgs & args)
{
    if(bEnableTimeline)
    {
        updateTimelineUIParams();
    }
    
    if(bUpdateSystem)
    {
        for(vector<ofx1DExtruder *>::iterator it = extruders.begin(); it != extruders.end(); ++it)
        {
            (*it)->update();
        }
        bgColor->setHsb(bgHue->getPos(), bgSat->getPos(), bgBri->getPos(), 255);
        bgColor2->setHsb(bgHue2->getPos(), bgSat2->getPos(), bgBri2->getPos(), 255);
        selfUpdate();
    }
	
	//Make this happen only when the timeline is modified by the user or when a new track is added.
	if(!ofGetMousePressed())
    {
		timeline->setOffset(ofVec2f(4, ofGetHeight() - timeline->getHeight() - 4 ));
		//timeline->setOffset(ofVec2f(4, 0 ));
		timeline->setWidth(ofGetWidth() - 8);
	}
}

void CloudsVisualSystem::draw(ofEventArgs & args)
{
    ofPushStyle();
    if(bRenderSystem)
    {
        CloudsVisualSystem::getSharedRenderTarget().begin();
		if(bClearBackground){
			ofClear(0, 0, 0, 0);
		}
        
        drawBackground();
        
		currentCamera->begin();
        
        ofRotateX(xRot->getPos());
        ofRotateY(yRot->getPos());
        ofRotateZ(zRot->getPos());
        
        selfSceneTransformation();
        
        glEnable(GL_DEPTH_TEST);
        
		ofPushStyle();
        drawDebug();
        ofPopStyle();
        
        lightsBegin();
        
		ofPushStyle();
        selfDraw();
        ofPopStyle();
		
        lightsEnd();
        
		currentCamera->end();
		
		ofPushStyle();
		ofPushMatrix();
		ofTranslate(0, ofGetHeight());
		ofScale(1,-1,1);
		
		selfDrawOverlay();
		
		ofPopMatrix();
		ofPopStyle();
		
        CloudsVisualSystem::getSharedRenderTarget().end();
		
		selfPostDraw();
		ofPushStyle();
		ofEnableAlphaBlending();
		getCursor().draw( ofGetMouseX(),ofGetMouseY() );
		ofPopStyle();
	}
    
	timeline->draw();
	
    ofPopStyle();
}

void CloudsVisualSystem::exit(ofEventArgs & args)
{
//    delete colorPalletes;
    delete bgColor;
    delete bgColor2;
    
    saveGUIS();
    
    for(vector<ofx1DExtruder *>::iterator it = extruders.begin(); it != extruders.end(); ++it)
    {
        ofx1DExtruder *e = (*it);
        delete e;
    }
    extruders.clear();
    
    for(map<string, ofxLight *>::iterator it = lights.begin(); it != lights.end(); ++it)
    {
        ofxLight *l = it->second;
        delete l;
    }
    lights.clear();
    
    for(map<string, ofMaterial *>::iterator it = materials.begin(); it != materials.end(); ++it)
    {
        ofMaterial *m = it->second;
        delete m;
    }
    materials.clear();
    materialGuis.clear();
    
    delete timeline;
    
    selfExit();
    
    deleteGUIS();
}


void CloudsVisualSystem::toggleControls(){
	
	toggleGUIS();
	timeline->toggleShow();
	
//	if(timeline->getIsShowing()){
//		ofShowCursor();
//	}
//	else{
//		ofHideCursor();
//	}
	
}
void CloudsVisualSystem::keyPressed(ofKeyEventArgs & args)
{
    for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
    {
        if((*it)->hasKeyboardFocus())
        {
            return;
        }
    }
	
    switch (args.key)
    {
        case '1':
            toggleGuiAndPosition(gui);
            break;
        case '2':
            toggleGuiAndPosition(sysGui);
            break;
        case '3':
            toggleGuiAndPosition(rdrGui);
            break;
        case '4':
            toggleGuiAndPosition(bgGui);
            break;
        case '5':
            toggleGuiAndPosition(lgtGui);
            break;
        case '0':
            toggleGuiAndPosition(camGui);
            break;
            
        case 'u':
        {
            bUpdateSystem = !bUpdateSystem;
        }
            break;
			
        case ' ':
        {
            ((ofxUIToggle *) tlGui->getWidget("ENABLE"))->setValue(timeline->getIsPlaying());
            ((ofxUIToggle *) tlGui->getWidget("ENABLE"))->triggerSelf();
        }
            break;
			
        case '`':
        {
            ofImage img;
            img.grabScreen(0,0,ofGetWidth(), ofGetHeight());
			if( !ofDirectory(getDataPath()+"snapshots/").exists() ){
				ofDirectory(getDataPath()+"snapshots/").create();
			}
            img.saveImage(getDataPath()+"snapshots/" + getSystemName() + " " + ofGetTimestampString() + ".png");
        }
            break;
            
        case 'h':
        {
			toggleControls();
        }
            break;
            
        case 'f':
        {
            ofToggleFullscreen();
        }
            break;
            
        case 'p':
        {
            for(int i = 0; i < guis.size(); i++)
            {
                guis[i]->setDrawWidgetPadding(true);
            }
        }
            break;
            
        case 'e':
        {
            ofxUISuperCanvas *last = NULL;
            for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
            {
                (*it)->setMinified(false);
                if(last != NULL)
                {
                    (*it)->getRect()->setX(last->getRect()->getX());
                    (*it)->getRect()->setY(last->getRect()->getY()+last->getRect()->getHeight()+1);
                    if((*it)->getRect()->getY()+(*it)->getRect()->getHeight() > ofGetHeight()-timeline->getHeight())
                    {
                        (*it)->getRect()->setX(last->getRect()->getX()+last->getRect()->getWidth()+1);
                        (*it)->getRect()->setY(1);
                    }
                }
                else
                {
                    (*it)->getRect()->setX(1);
                    (*it)->getRect()->setY(1);
                }
                last = (*it);
            }
        }
            break;
            
        case 'r':
        {
            ofxUISuperCanvas *last = NULL;
            for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
            {
                if(last != NULL)
                {
                    (*it)->getRect()->setX(last->getRect()->getX()+last->getRect()->getWidth()+1);
                    (*it)->getRect()->setY(1);
                }
                else
                {
                    (*it)->getRect()->setX(1);
                    (*it)->getRect()->setY(1);
                }
                last = (*it);
                last->setMinified(false);
            }
        }
            break;
            
        case 't':
        {
            ofxUISuperCanvas *last = NULL;
            for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
            {
                (*it)->setMinified(true);
                if(last != NULL)
                {
                    (*it)->getRect()->setX(1);
                    (*it)->getRect()->setY(last->getRect()->getY()+last->getRect()->getHeight()+1);
                }
                else
                {
                    (*it)->getRect()->setX(1);
                    (*it)->getRect()->setY(1);
                }
                last = (*it);
            }
        }
            break;
            
        case 'y':
        {
            float x = ofGetWidth()*.5;
            float y = ofGetHeight()*.5;
            float tempRadius = gui->getGlobalCanvasWidth()*2.0;
            float stepSize = TWO_PI/(float)guis.size();
            float theta = 0;
            for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
            {
                (*it)->getRect()->setX(x+sin(theta)*tempRadius - (*it)->getRect()->getHalfWidth());
                (*it)->getRect()->setY(y+cos(theta)*tempRadius - (*it)->getRect()->getHalfHeight());
                theta +=stepSize;
            }
        }
            break;
            
        case '=':
        {
            for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
            {
                (*it)->toggleMinified();
            }
        }
            break;
            
        default:
            selfKeyPressed(args);
            break;
    }
}

void CloudsVisualSystem::keyReleased(ofKeyEventArgs & args)
{
    switch (args.key)
    {
        case 'p':
        {
            for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
            {
                (*it)->setDrawWidgetPadding(false);
            }
        }
            break;
            
            
        default:
            selfKeyReleased(args);
            break;
    }
}

void CloudsVisualSystem::mouseDragged(ofMouseEventArgs& data)
{
    selfMouseDragged(data);
}

void CloudsVisualSystem::mouseMoved(ofMouseEventArgs& data)
{
    selfMouseMoved(data);
}

void CloudsVisualSystem::mousePressed(ofMouseEventArgs & args)
{
	if(cursorIsOverGUI()){
		cam.disableMouseInput();
	}
    selfMousePressed(args);
}

bool CloudsVisualSystem::cursorIsOverGUI(){
	if(timeline->getDrawRect().inside(ofGetMouseX(),ofGetMouseY())){
		return true;
	}
	
    for(int i = 0; i < guis.size(); i++)
    {
		
		if(guis[i]->isHit(ofGetMouseX(), ofGetMouseY()))
		{
			return true;
		}
	}
	return false;
}

void CloudsVisualSystem::mouseReleased(ofMouseEventArgs & args)
{
    cam.enableMouseInput();
    selfMouseReleased(args);
}

void CloudsVisualSystem::setupAppParams()
{
//	colorPalletes = new ofxColorPalettes(getDataPath()+"colors/");
    ofSetSphereResolution(30);
    bRenderSystem = true;
    bUpdateSystem = true;
}

void CloudsVisualSystem::setupDebugParams()
{
    //DEBUG
    bDebug = true;
    debugGridSize = 100;
}

void CloudsVisualSystem::setupCameraParams()
{
    //CAMERA
    camFOV = 60;
    camDistance = 200;
    cam.setDistance(camDistance);
    cam.setFov(camFOV);
	//    cam.setForceAspectRatio(true);
	//    bgAspectRatio = (float)ofGetWidth()/(float)ofGetHeight();
	//    cam.setAspectRatio(bgAspectRatio);
    xRot = new ofx1DExtruder(0);
    yRot = new ofx1DExtruder(0);
    zRot = new ofx1DExtruder(0);
    xRot->setPhysics(.9, 5.0, 25.0);
    yRot->setPhysics(.9, 5.0, 25.0);
    zRot->setPhysics(.9, 5.0, 25.0);
    extruders.push_back(xRot);
    extruders.push_back(yRot);
    extruders.push_back(zRot);
}

void CloudsVisualSystem::setupLightingParams()
{
    //LIGHTING
    bSmoothLighting = true;
    bEnableLights = true;
    globalAmbientColor = new float[4];
    globalAmbientColor[0] = 0.5;
    globalAmbientColor[1] = 0.5;
    globalAmbientColor[2] = 0.5;
    globalAmbientColor[3] = 1.0;
}

void CloudsVisualSystem::setupMaterialParams()
{
    mat = new ofMaterial();
}

void CloudsVisualSystem::setupTimeLineParams()
{
    bShowTimeline = true;
    bDeleteTimelineTrack = false;
    timelineDuration = 1000;
    bEnableTimeline = false;
    bEnableTimelineTrackCreation = false;
}

void CloudsVisualSystem::setupCoreGuis()
{
    setupGui();
    setupSystemGui();
    setupRenderGui();
    setupBackgroundGui();
    setupLightingGui();
    setupCameraGui();
    setupMaterial("MATERIAL 1", mat);
    setupPointLight("POINT LIGHT 1");
    setupPresetGui();
}

void CloudsVisualSystem::setupGui()
{
    gui = new ofxUISuperCanvas(ofToUpper(getSystemName()));
    gui->setName("Settings");
    gui->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    ofxUIFPS *fps = gui->addFPS();
    gui->resetPlacer();
    gui->addWidgetDown(fps, OFX_UI_ALIGN_RIGHT, true);
    gui->addWidgetToHeader(fps);
    
    gui->addSpacer();
    gui->addButton("SAVE", false);
    gui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    ofxUIButton *loadbtn = gui->addButton("LOAD", false);
    gui->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    ofxUIButton *updatebtn = gui->addToggle("UPDATE", &bUpdateSystem);
    gui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    ofxUIButton *renderbtn = gui->addToggle("RENDER", &bRenderSystem);
    gui->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    gui->addWidgetNorthOf(loadbtn, "RENDER", true);
    gui->setPlacer(updatebtn);
    gui->addSpacer();
    selfSetupGui();
    gui->autoSizeToFitWidgets();
    ofAddListener(gui->newGUIEvent,this,&CloudsVisualSystem::guiEvent);
    guis.push_back(gui);
    guimap[gui->getName()] = gui;
}

vector<string> CloudsVisualSystem::getPresets()
{
	vector<string> presets;
	ofDirectory presetsFolder = ofDirectory(getVisualSystemDataPath());
	if(presetsFolder.exists()){
		presetsFolder.listDir();
		for(int i = 0; i < presetsFolder.size(); i++){
			if(presetsFolder.getFile(i).isDirectory() &&
               ofFilePath::removeTrailingSlash(presetsFolder.getName(i)) != "Working" &&
			   presetsFolder.getName(i).at(0) != '_') //use leading _ to hide folders
            {
				presets.push_back(presetsFolder.getName(i));
			}
		}
	}
	return presets;
}

void CloudsVisualSystem::guiEvent(ofxUIEventArgs &e)
{
    string name = e.widget->getName();
    if(name == "SAVE")
    {
        ofxUIButton *b = (ofxUIButton *) e.widget;
        if(b->getValue())
        {
            string presetName = ofSystemTextBoxDialog("Save Preset As");
            if(presetName.length())
            {
                savePresetGUIS(presetName);
            }
            else{
                saveGUIS();
            }
        }
    }
    else if(name == "LOAD")
    {
        ofxUIButton *b = (ofxUIButton *) e.widget;
        if(b->getValue())
        {
            ofFileDialogResult result = ofSystemLoadDialog("Load Visual System Preset Folder", true, getVisualSystemDataPath());
            if(result.bSuccess && result.fileName.length())
            {
                loadPresetGUISFromPath(result.filePath);
            }
            else{
                loadGUIS();
            }
        }
    }
	
    selfGuiEvent(e);
}

void CloudsVisualSystem::setupSystemGui()
{
    sysGui = new ofxUISuperCanvas("SYSTEM", gui);
    sysGui->copyCanvasStyle(gui);
    sysGui->copyCanvasProperties(gui);
    sysGui->setPosition(guis[guis.size()-1]->getRect()->x+guis[guis.size()-1]->getRect()->getWidth()+1, 0);
    sysGui->setName("SystemSettings");
    sysGui->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    ofxUIToggle *toggle = sysGui->addToggle("DEBUG", &bDebug);
    toggle->setLabelPosition(OFX_UI_WIDGET_POSITION_LEFT);
    sysGui->resetPlacer();
    sysGui->addWidgetDown(toggle, OFX_UI_ALIGN_RIGHT, true);
    sysGui->addWidgetToHeader(toggle);
    sysGui->addSpacer();
    
    selfSetupSystemGui();
    sysGui->autoSizeToFitWidgets();
    ofAddListener(sysGui->newGUIEvent,this,&CloudsVisualSystem::guiSystemEvent);
    guis.push_back(sysGui);
    guimap[sysGui->getName()] = sysGui;
}

void CloudsVisualSystem::setupRenderGui()
{
    rdrGui = new ofxUISuperCanvas("RENDER", gui);
    rdrGui->copyCanvasStyle(gui);
    rdrGui->copyCanvasProperties(gui);
    rdrGui->setPosition(guis[guis.size()-1]->getRect()->x+guis[guis.size()-1]->getRect()->getWidth()+1, 0);
    rdrGui->setName("RenderSettings");
    rdrGui->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    rdrGui->addSpacer();
    selfSetupRenderGui();
    
    rdrGui->autoSizeToFitWidgets();
    ofAddListener(rdrGui->newGUIEvent,this,&CloudsVisualSystem::guiRenderEvent);
    guis.push_back(rdrGui);
    guimap[rdrGui->getName()] = rdrGui;
}

void CloudsVisualSystem::setupBackgroundGui()
{
    bgHue = new ofx1DExtruder(0);
	bgSat = new ofx1DExtruder(0);
	bgBri = new ofx1DExtruder(0);
	
	bgHue->setPhysics(.95, 5.0, 25.0);
	bgSat->setPhysics(.95, 5.0, 25.0);
	bgBri->setPhysics(.95, 5.0, 25.0);
	
    bgHue2 = new ofx1DExtruder(0);
	bgSat2 = new ofx1DExtruder(0);
	bgBri2 = new ofx1DExtruder(0);
	
	bgHue2->setPhysics(.95, 5.0, 25.0);
	bgSat2->setPhysics(.95, 5.0, 25.0);
	bgBri2->setPhysics(.95, 5.0, 25.0);
    
	gradientMode = 0;
    bgHue->setHome((330.0/360.0)*255.0);
	bgSat->setHome(0);
	bgBri->setHome(0);
    bgColor = new ofColor(0,0,0);
    
    bgHue2->setHome((330.0/360.0)*255.0);
	bgSat2->setHome(0);
	bgBri2->setHome(0);
	bgColor2 = new ofColor(0,0,0);
    
    extruders.push_back(bgHue);
    extruders.push_back(bgSat);
    extruders.push_back(bgBri);
    
    extruders.push_back(bgHue2);
    extruders.push_back(bgSat2);
    extruders.push_back(bgBri2);
    
    bgGui = new ofxUISuperCanvas("BACKGROUND", gui);
    bgGui->copyCanvasStyle(gui);
    bgGui->copyCanvasProperties(gui);
    bgGui->setPosition(guis[guis.size()-1]->getRect()->x+guis[guis.size()-1]->getRect()->getWidth()+1, 0);
    bgGui->setName("BgSettings");
    bgGui->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    ofxUIToggle *toggle = bgGui->addToggle("GRAD", false);
    toggle->setLabelPosition(OFX_UI_WIDGET_POSITION_LEFT);
    bgGui->resetPlacer();
    bgGui->addWidgetDown(toggle, OFX_UI_ALIGN_RIGHT, true);
    bgGui->addWidgetToHeader(toggle);
    bgGui->addSpacer();
    
    bgGui->addSlider("HUE", 0.0, 255.0, bgHue->getPosPtr());
    bgGui->addSlider("SAT", 0.0, 255.0, bgSat->getPosPtr());
    bgGui->addSlider("BRI", 0.0, 255.0, bgBri->getPosPtr());
    bgGui->addSpacer();
    hueSlider = bgGui->addSlider("HUE2", 0.0, 255.0, bgHue2->getPosPtr());
    satSlider = bgGui->addSlider("SAT2", 0.0, 255.0, bgSat2->getPosPtr());
    briSlider = bgGui->addSlider("BRI2", 0.0, 255.0, bgBri2->getPosPtr());
    bgGui->autoSizeToFitWidgets();
    ofAddListener(bgGui->newGUIEvent,this,&CloudsVisualSystem::guiBackgroundEvent);
    guis.push_back(bgGui);
    guimap[bgGui->getName()] = bgGui;
}

void CloudsVisualSystem::guiBackgroundEvent(ofxUIEventArgs &e)
{
    string name = e.widget->getName();
    
    if(name == "BRI")
    {
        bgBri->setPosAndHome(bgBri->getPos());
        for(int i = 0; i < guis.size(); i++)
        {
            guis[i]->setWidgetColor(OFX_UI_WIDGET_COLOR_BACK, ofColor(bgBri->getPos(),OFX_UI_COLOR_BACK_ALPHA*REZANATOR_GUI_ALPHA_MULTIPLIER));
            guis[i]->setColorBack(ofColor(255-bgBri->getPos(), OFX_UI_COLOR_BACK_ALPHA*REZANATOR_GUI_ALPHA_MULTIPLIER));
        }
    }
    else if(name == "SAT")
    {
        bgSat->setPosAndHome(bgSat->getPos());
    }
    else if(name == "HUE")
    {
        bgHue->setPosAndHome(bgHue->getPos());
    }
    else if(name == "BRI2")
    {
        bgBri2->setPosAndHome(bgBri2->getPos());
    }
    else if(name == "SAT2")
    {
        bgSat2->setPosAndHome(bgSat2->getPos());
    }
    else if(name == "HUE2")
    {
        bgHue2->setPosAndHome(bgHue2->getPos());
    }
    else if(name == "GRAD")
    {
        ofxUIToggle *t = (ofxUIToggle *) e.widget;
        if(t->getValue())
        {
            gradientMode = OF_GRADIENT_CIRCULAR;
            hueSlider->setVisible(true);
            satSlider->setVisible(true);
            briSlider->setVisible(true);
            bgGui->autoSizeToFitWidgets();
            if(bgGui->isMinified())
            {
                bgGui->setMinified(true);
            }
        }
        else
        {
            gradientMode = -1;
            hueSlider->setVisible(false);
            satSlider->setVisible(false);
            briSlider->setVisible(false);
            bgGui->autoSizeToFitWidgets();
        }
    }
}

void CloudsVisualSystem::setupLightingGui()
{
    lgtGui = new ofxUISuperCanvas("LIGHT", gui);
    lgtGui->copyCanvasStyle(gui);
    lgtGui->copyCanvasProperties(gui);
    lgtGui->setName("LightSettings");
    lgtGui->setPosition(guis[guis.size()-1]->getRect()->x+guis[guis.size()-1]->getRect()->getWidth()+1, 0);
    lgtGui->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    ofxUIToggle *toggle = lgtGui->addToggle("ENABLE", &bEnableLights);
    toggle->setLabelPosition(OFX_UI_WIDGET_POSITION_LEFT);
    lgtGui->resetPlacer();
    lgtGui->addWidgetDown(toggle, OFX_UI_ALIGN_RIGHT, true);
    lgtGui->addWidgetToHeader(toggle);
    
    lgtGui->addSpacer();
    lgtGui->addToggle("SMOOTH", &bSmoothLighting);
    lgtGui->addSpacer();
    float length = (lgtGui->getGlobalCanvasWidth()-lgtGui->getWidgetSpacing()*5)/3.;
    float dim = lgtGui->getGlobalSliderHeight();
    lgtGui->addLabel("GLOBAL AMBIENT COLOR", OFX_UI_FONT_SMALL);
    lgtGui->addMinimalSlider("R", 0.0, 1.0, &globalAmbientColor[0], length, dim)->setShowValue(false);
    lgtGui->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    lgtGui->addMinimalSlider("G", 0.0, 1.0, &globalAmbientColor[1], length, dim)->setShowValue(false);
    lgtGui->addMinimalSlider("B", 0.0, 1.0, &globalAmbientColor[2], length, dim)->setShowValue(false);
    lgtGui->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    lgtGui->autoSizeToFitWidgets();
    ofAddListener(lgtGui->newGUIEvent,this,&CloudsVisualSystem::guiLightingEvent);
    guis.push_back(lgtGui);
    guimap[lgtGui->getName()] = lgtGui;
}

void CloudsVisualSystem::guiLightingEvent(ofxUIEventArgs &e)
{
    string name = e.widget->getName();
    if(name == "R")
    {
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbientColor);
    }
    else if(name == "G")
    {
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbientColor);
    }
    else if(name == "B")
    {
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, globalAmbientColor);
    }
}

void CloudsVisualSystem::setupCameraGui()
{
    camGui = new ofxUISuperCanvas("CAMERA", gui);
    camGui->copyCanvasStyle(gui);
    camGui->copyCanvasProperties(gui);
    camGui->setName("CamSettings");
    camGui->setPosition(guis[guis.size()-1]->getRect()->x+guis[guis.size()-1]->getRect()->getWidth()+1, 0);
    camGui->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    ofxUIButton *button = camGui->addButton("RESET", false);
    button->setLabelPosition(OFX_UI_WIDGET_POSITION_LEFT);
    camGui->resetPlacer();
    camGui->addWidgetDown(button, OFX_UI_ALIGN_RIGHT, true);
    camGui->addWidgetToHeader(button);
    camGui->addSpacer();
    camGui->addSlider("DIST", 0, 1000, &camDistance);
    camGui->addSlider("FOV", 0, 180, &camFOV);
    camGui->addSlider("ROT-X", 0, 360.0, xRot->getPosPtr())->setIncrement(1.0);
    camGui->addSlider("ROT-Y", 0, 360.0, yRot->getPosPtr())->setIncrement(1.0);
    camGui->addSlider("ROT-Z", 0, 360.0, zRot->getPosPtr())->setIncrement(1.0);
    
    camGui->addSpacer();
    vector<string> views;
    views.push_back("TOP");
    views.push_back("BOTTOM");
    views.push_back("FRONT");
    views.push_back("BACK");
    views.push_back("RIGHT");
    views.push_back("LEFT");
    views.push_back("3D");
    ofxUIDropDownList *ddl = camGui->addDropDownList("VIEW", views);
    ddl->setAutoClose(false);
    ddl->setShowCurrentSelected(true);
    
    camGui->autoSizeToFitWidgets();
    ofAddListener(camGui->newGUIEvent,this,&CloudsVisualSystem::guiCameraEvent);
    guis.push_back(camGui);
    guimap[camGui->getName()] = camGui;
}

void CloudsVisualSystem::guiCameraEvent(ofxUIEventArgs &e)
{
    string name = e.widget->getName();
    if(name == "DIST")
    {
        cam.setDistance(camDistance);
		//		currentCamera->setDistance(camDistance);
    }
    else if(name == "FOV")
    {
		//		currentCamera->setFov(camFOV);
        cam.setFov(camFOV);
    }
    else if(name == "ROT-X")
    {
        xRot->setPosAndHome(xRot->getPos());
    }
    else if(name == "ROT-Y")
    {
        yRot->setPosAndHome(yRot->getPos());
    }
    else if(name == "ROT-Z")
    {
        zRot->setPosAndHome(zRot->getPos());
    }
    else if(name == "RESET")
    {
        ofxUIButton *b = (ofxUIButton *) e.widget;
        if(b->getValue())
        {
            cam.reset();
        }
    }
    else if(name == "TOP")
    {
        ofxUIToggle *t = (ofxUIToggle *) e.widget;
        if(t->getValue())
        {
            xRot->setHome(0);
            yRot->setHome(0);
            zRot->setHome(0);
        }
    }
    else if(name == "BOTTOM")
    {
        ofxUIToggle *t = (ofxUIToggle *) e.widget;
        if(t->getValue())
        {
            xRot->setHome(-180);
            yRot->setHome(0);
            zRot->setHome(0);
        }
    }
    else if(name == "BACK")
    {
        ofxUIToggle *t = (ofxUIToggle *) e.widget;
        if(t->getValue())
        {
            xRot->setHome(-90);
            yRot->setHome(0);
            zRot->setHome(180);
        }
    }
    else if(name == "FRONT")
    {
        ofxUIToggle *t = (ofxUIToggle *) e.widget;
        if(t->getValue())
        {
            xRot->setHome(-90);
            yRot->setHome(0);
            zRot->setHome(0);
        }
    }
    else if(name == "RIGHT")
    {
        ofxUIToggle *t = (ofxUIToggle *) e.widget;
        if(t->getValue())
        {
            xRot->setHome(-90);
            yRot->setHome(0);
            zRot->setHome(90);
        }
    }
    else if(name == "LEFT")
    {
        ofxUIToggle *t = (ofxUIToggle *) e.widget;
        if(t->getValue())
        {
            xRot->setHome(-90);
            yRot->setHome(0);
            zRot->setHome(-90);
            
        }
    }
    else if(name == "3D")
    {
        ofxUIToggle *t = (ofxUIToggle *) e.widget;
        if(t->getValue())
        {
            xRot->setHome(-70);
            yRot->setHome(0);
            zRot->setHome(45);
        }
    }
}

void CloudsVisualSystem::setupPresetGui()
{
	presetGui = new ofxUISuperCanvas("PRESETS");
    presetGui->setTriggerWidgetsUponLoad(false);
	presetGui->setName("Presets");
	presetGui->copyCanvasStyle(gui);
    presetGui->copyCanvasProperties(gui);
    presetGui->addSpacer();
    
    vector<string> empty; empty.clear();
	presetRadio = presetGui->addRadio("PRESETS", empty);
	
	presetGui->setWidgetFontSize(OFX_UI_FONT_SMALL);
    vector<string> presets = getPresets();
    for(vector<string>::iterator it = presets.begin(); it != presets.end(); ++it)
    {
        ofxUIToggle *t = presetGui->addToggle((*it), false);
        presetRadio->addToggle(t);
    }
	
	presetGui->autoSizeToFitWidgets();
    ofAddListener(presetGui->newGUIEvent,this,&CloudsVisualSystem::guiPresetEvent);
    guis.push_back(presetGui);
    guimap[presetGui->getName()] = presetGui;
}

void CloudsVisualSystem::guiPresetEvent(ofxUIEventArgs &e)
{
    ofxUIToggle *t = (ofxUIToggle *) e.widget;
    if(t->getValue())
    {
        loadPresetGUISFromName(e.widget->getName());
    }
}

void CloudsVisualSystem::setupMaterial(string name, ofMaterial *m)
{
    materials[name] = m;
    ofxUISuperCanvas* g = new ofxUISuperCanvas(name, gui);
    materialGuis[name] = g;
    g->copyCanvasStyle(gui);
    g->copyCanvasProperties(gui);
    g->setName(name+"Settings");
    g->addSpacer();
    g->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    float length = (g->getGlobalCanvasWidth()-g->getWidgetSpacing()*5)/3.;
    float dim = g->getGlobalSliderHeight();
    
    g->addLabel("AMBIENT", OFX_UI_FONT_SMALL);
    g->addMinimalSlider("AR", 0.0, 1.0, &m->getAmbientColor().r, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    g->addMinimalSlider("AG", 0.0, 1.0, &m->getAmbientColor().g, length, dim)->setShowValue(false);
    g->addMinimalSlider("AB", 0.0, 1.0, &m->getAmbientColor().b, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    g->addSpacer();
    
    g->addLabel("DIFFUSE", OFX_UI_FONT_SMALL);
    g->addMinimalSlider("AR", 0.0, 1.0, &m->getDiffuseColor().r, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    g->addMinimalSlider("AG", 0.0, 1.0, &m->getDiffuseColor().g, length, dim)->setShowValue(false);
    g->addMinimalSlider("AB", 0.0, 1.0, &m->getDiffuseColor().b, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    g->addSpacer();
    
    g->addLabel("EMISSIVE", OFX_UI_FONT_SMALL);
    g->addMinimalSlider("ER", 0.0, 1.0, &m->getEmissiveColor().r, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    g->addMinimalSlider("EG", 0.0, 1.0, &m->getEmissiveColor().g, length, dim)->setShowValue(false);
    g->addMinimalSlider("EB", 0.0, 1.0, &m->getEmissiveColor().b, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    g->addSpacer();
    
    g->addLabel("SPECULAR", OFX_UI_FONT_SMALL);
    g->addMinimalSlider("SR", 0.0, 1.0, &m->getSpecularColor().r, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    g->addMinimalSlider("SG", 0.0, 1.0, &m->getSpecularColor().g, length, dim)->setShowValue(false);
    g->addMinimalSlider("SB", 0.0, 1.0, &m->getSpecularColor().b, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    g->addSpacer();
    
    g->addMinimalSlider("SHINY", 0.0, 128.0, &(m->getShininess()))->setShowValue(false);
    
    g->autoSizeToFitWidgets();
    g->setPosition(ofGetWidth()*.5-g->getRect()->getHalfWidth(), ofGetHeight()*.5 - g->getRect()->getHalfHeight());
    
    ofAddListener(g->newGUIEvent,this,&CloudsVisualSystem::guiMaterialEvent);
    guis.push_back(g);
    guimap[g->getName()] = g;
}

void CloudsVisualSystem::guiMaterialEvent(ofxUIEventArgs &e)
{
    
}

void CloudsVisualSystem::setupPointLight(string name)
{
    ofxLight *l = new ofxLight();
    l->light.setPointLight();
	//removes light until we are active
	l->light.destroy();
	
    lights[name] = l;
    
    ofxUISuperCanvas* g = new ofxUISuperCanvas(name, gui);
    lightGuis[name] = g;
    g->copyCanvasStyle(gui);
    g->copyCanvasProperties(gui);
    g->setName(name+"Settings");
    g->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    ofxUIToggle *toggle = g->addToggle("ENABLE", &l->bEnabled);
    toggle->setLabelPosition(OFX_UI_WIDGET_POSITION_LEFT);
    g->resetPlacer();
    g->addWidgetDown(toggle, OFX_UI_ALIGN_RIGHT, true);
    g->addWidgetToHeader(toggle);
    g->addSpacer();
    
    setupGenericLightProperties(g, l);
    
    g->autoSizeToFitWidgets();
    g->setPosition(ofGetWidth()*.5-g->getRect()->getHalfWidth(), ofGetHeight()*.5 - g->getRect()->getHalfHeight());
    
    ofAddListener(g->newGUIEvent,this,&CloudsVisualSystem::guiLightEvent);
    guis.push_back(g);
    guimap[g->getName()] = g;
}

void CloudsVisualSystem::setupSpotLight(string name)
{
    ofxLight *l = new ofxLight();
    l->light.setSpotlight();
	l->light.destroy();
	
    lights[name] = l;
    
    ofxUISuperCanvas* g = new ofxUISuperCanvas(name, gui);
    lightGuis[name] = g;
    g->copyCanvasStyle(gui);
    g->copyCanvasProperties(gui);
    g->setName(name+"Settings");
    g->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    ofxUIToggle *toggle = g->addToggle("ENABLE", &l->bEnabled);
    toggle->setLabelPosition(OFX_UI_WIDGET_POSITION_LEFT);
    g->resetPlacer();
    g->addWidgetDown(toggle, OFX_UI_ALIGN_RIGHT, true);
    g->addWidgetToHeader(toggle);
    g->addSpacer();
    
    g->addSlider("CUT OFF", 0, 90, &l->lightSpotCutOff);
    g->addSlider("EXPONENT", 0.0, 128.0, &l->lightExponent);
    g->addSpacer();
    
    setupGenericLightProperties(g, l);
    
    g->autoSizeToFitWidgets();
    g->setPosition(ofGetWidth()*.5-g->getRect()->getHalfWidth(), ofGetHeight()*.5 - g->getRect()->getHalfHeight());
    
    ofAddListener(g->newGUIEvent,this,&CloudsVisualSystem::guiLightEvent);
    guis.push_back(g);
    guimap[g->getName()] = g;
}

void CloudsVisualSystem::setupBeamLight(string name)
{
    ofxLight *l = new ofxLight();
    l->light.setDirectional();
	l->light.destroy();
	
    lights[name] = l;
    
    ofxUISuperCanvas* g = new ofxUISuperCanvas(name, gui);
    lightGuis[name] = g;
    g->copyCanvasStyle(gui);
    g->copyCanvasProperties(gui);
    g->setName(name+"Settings");
    g->setWidgetFontSize(OFX_UI_FONT_SMALL);
    
    ofxUIToggle *toggle = g->addToggle("ENABLE", &l->bEnabled);
    toggle->setLabelPosition(OFX_UI_WIDGET_POSITION_LEFT);
    g->resetPlacer();
    g->addWidgetDown(toggle, OFX_UI_ALIGN_RIGHT, true);
    g->addWidgetToHeader(toggle);
    g->addSpacer();
    
    setupGenericLightProperties(g, l);
    
    g->autoSizeToFitWidgets();
    g->setPosition(ofGetWidth()*.5-g->getRect()->getHalfWidth(), ofGetHeight()*.5 - g->getRect()->getHalfHeight());
    
    ofAddListener(g->newGUIEvent,this,&CloudsVisualSystem::guiLightEvent);
    guis.push_back(g);
    guimap[g->getName()] = g;
}

void CloudsVisualSystem::setupGenericLightProperties(ofxUISuperCanvas *g, ofxLight *l)
{
    float length = (g->getGlobalCanvasWidth()-g->getWidgetSpacing()*5)/3.;
    float dim = g->getGlobalSliderHeight();
    
    switch(l->light.getType())
    {
        case OF_LIGHT_POINT:
        {
            g->addLabel("POSITION", OFX_UI_FONT_SMALL);
            g->addMinimalSlider("X", -1000.0, 1000.0, &l->lightPos.x, length, dim)->setShowValue(false);
            g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
            g->addMinimalSlider("Y", -1000.0, 1000.0, &l->lightPos.y, length, dim)->setShowValue(false);
            g->addMinimalSlider("Z", -1000.0, 1000.0, &l->lightPos.z, length, dim)->setShowValue(false);
            g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
            g->addSpacer();
        }
            break;
            
        case OF_LIGHT_SPOT:
        {
            g->addLabel("POSITION", OFX_UI_FONT_SMALL);
            g->addMinimalSlider("X", -1000.0, 1000.0, &l->lightPos.x, length, dim)->setShowValue(false);
            g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
            g->addMinimalSlider("Y", -1000.0, 1000.0, &l->lightPos.y, length, dim)->setShowValue(false);
            g->addMinimalSlider("Z", -1000.0, 1000.0, &l->lightPos.z, length, dim)->setShowValue(false);
            g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
            g->addSpacer();
            
            g->addLabel("ORIENTATION", OFX_UI_FONT_SMALL);
            g->addMinimalSlider("OX", -90.0, 90.0, &l->lightOrientation.x, length, dim)->setShowValue(false);
            g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
            g->addMinimalSlider("OY", -90.0, 90.0, &l->lightOrientation.y, length, dim)->setShowValue(false);
            g->addMinimalSlider("OZ", -90.0, 90.0, &l->lightOrientation.z, length, dim)->setShowValue(false);
            g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
            g->addSpacer();
        }
            break;
            
        case OF_LIGHT_DIRECTIONAL:
        {
            g->addLabel("ORIENTATION", OFX_UI_FONT_SMALL);
            g->addMinimalSlider("OX", -90.0, 90.0, &l->lightOrientation.x, length, dim)->setShowValue(false);
            g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
            g->addMinimalSlider("OY", -90.0, 90.0, &l->lightOrientation.y, length, dim)->setShowValue(false);
            g->addMinimalSlider("OZ", -90.0, 90.0, &l->lightOrientation.z, length, dim)->setShowValue(false);
            g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
            g->addSpacer();
        }
            break;
    }
    
    g->addLabel("AMBIENT", OFX_UI_FONT_SMALL);
    g->addMinimalSlider("AR", 0.0, 1.0, &l->lightAmbient.r, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    g->addMinimalSlider("AG", 0.0, 1.0, &l->lightAmbient.g, length, dim)->setShowValue(false);
    g->addMinimalSlider("AB", 0.0, 1.0, &l->lightAmbient.b, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    
    g->addSpacer();
    g->addLabel("DIFFUSE", OFX_UI_FONT_SMALL);
    g->addMinimalSlider("DR", 0.0, 1.0, &l->lightDiffuse.r, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    g->addMinimalSlider("DG", 0.0, 1.0, &l->lightDiffuse.g, length, dim)->setShowValue(false);
    g->addMinimalSlider("DB", 0.0, 1.0, &l->lightDiffuse.b, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    
    g->addSpacer();
    g->addLabel("SPECULAR", OFX_UI_FONT_SMALL);
    g->addMinimalSlider("SR", 0.0, 1.0, &l->lightSpecular.r, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_RIGHT);
    g->addMinimalSlider("SG", 0.0, 1.0, &l->lightSpecular.g, length, dim)->setShowValue(false);
    g->addMinimalSlider("SB", 0.0, 1.0, &l->lightSpecular.b, length, dim)->setShowValue(false);
    g->setWidgetPosition(OFX_UI_WIDGET_POSITION_DOWN);
    g->addSpacer();
}

void CloudsVisualSystem::guiLightEvent(ofxUIEventArgs &e)
{
    
}

void CloudsVisualSystem::setupTimeline()
{

    timeline = new ofxTimeline();
	timeline->setup();
    timeline->setMinimalHeaders(true);
	timeline->setDurationInFrames(1000);
	timeline->setLoopType(OF_LOOP_NORMAL);
    timeline->setPageName(ofToUpper(getSystemName()));
    
    ofDirectory dir;
    string workingDirectoryName = getVisualSystemDataPath()+"Working/Timeline/";
    if(!dir.doesDirectoryExist(workingDirectoryName))
    {
        dir.createDirectory(workingDirectoryName);
    }
    
    timeline->setWorkingFolder(getVisualSystemDataPath()+"Working/Timeline/");
    ofAddListener(timeline->events().bangFired, this, &CloudsVisualSystem::timelineBangEvent);
    
    selfSetupTimeline();
}

void CloudsVisualSystem::resetTimeline()
{
	ofRemoveListener(timeline->events().bangFired, this, &CloudsVisualSystem::timelineBangEvent);
    timeline->reset();
    timeline->setPageName(ofToUpper(getSystemName()));
    setupTimeline();
}

void CloudsVisualSystem::timelineBangEvent(ofxTLBangEventArgs& args)
{
    if(bEnableTimeline)
    {
        map<ofxTLBangs*, ofxUIButton*>::iterator it = tlButtonMap.find((ofxTLBangs *)args.track);
        if(it != tlButtonMap.end())
        {
            ofxUIButton *b = it->second;
            b->setValue(!b->getValue());
            b->triggerSelf();
            b->setValue(!b->getValue());
        }
    }
}

void CloudsVisualSystem::setupTimelineGui()
{
    tlGui = new ofxUISuperCanvas("TIMELINE", gui);
    tlGui->copyCanvasStyle(gui);
    tlGui->copyCanvasProperties(gui);
    tlGui->setPosition(guis[guis.size()-1]->getRect()->x+guis[guis.size()-1]->getRect()->getWidth()+1, 0);
    tlGui->setName("TimelineSettings");
    tlGui->setWidgetFontSize(OFX_UI_FONT_SMALL);
	
    ofxUIToggle *toggle = tlGui->addToggle("ENABLE", &bEnableTimeline);
    toggle->setLabelPosition(OFX_UI_WIDGET_POSITION_LEFT);
    tlGui->resetPlacer();
    tlGui->addWidgetDown(toggle, OFX_UI_ALIGN_RIGHT, true);
    tlGui->addWidgetToHeader(toggle);
    tlGui->addSpacer();
    
    tlGui->addNumberDialer("DURATION", 0.0, 10000, &timelineDuration, 0.0)->setDisplayLabel(true);
    
    tlGui->addToggle("ANIMATE", &bEnableTimelineTrackCreation);
    tlGui->addToggle("DELETE", &bDeleteTimelineTrack);
    
    tlGui->addToggle("SHOW/HIDE", &bShowTimeline);
    
    selfSetupTimelineGui();
    tlGui->autoSizeToFitWidgets();
    ofAddListener(tlGui->newGUIEvent,this,&CloudsVisualSystem::guiTimelineEvent);
    guis.push_back(tlGui);
    guimap[tlGui->getName()] = tlGui;
}

void CloudsVisualSystem::guiTimelineEvent(ofxUIEventArgs &e)
{
    string name = e.widget->getName();
    if(name == "DURATION")
    {
        timeline->setDurationInFrames(floor(timelineDuration));
    }
    else if(name == "ANIMATE")
    {
        ofxUIToggle *t = (ofxUIToggle *) e.widget;
        if(t->getValue())
        {
            setTimelineTrackDeletion(false);
        }
        setTimelineTrackCreation(t->getValue());
    }
    else if(name == "DELETE")
    {
        ofxUIToggle *t = (ofxUIToggle *) e.widget;
        if(t->getValue())
        {
            setTimelineTrackCreation(false);
        }
        setTimelineTrackDeletion(t->getValue());
        
    }
    else if(name == "ENABLE")
    {
        if(bEnableTimeline)
        {
            if(bEnableTimelineTrackCreation)
            {
                setTimelineTrackCreation(false);
            }
            if(bDeleteTimelineTrack)
            {
                setTimelineTrackDeletion(false);
            }
        }
    }
    else if(name == "SHOW/HIDE")
    {
        if(bShowTimeline)
        {
            timeline->show();
        }
        else
        {
            timeline->hide();
        }
    }
}

void CloudsVisualSystem::setTimelineTrackDeletion(bool state)
{
    bDeleteTimelineTrack = state;
    if(bDeleteTimelineTrack)
    {
        bEnableTimeline = false;
        for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
        {
            if((*it)->getName() != "TimelineSettings")
            {
                ofAddListener((*it)->newGUIEvent,this,&CloudsVisualSystem::guiAllEvents);
            }
        }
    }
    else
    {
        for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
        {
            if((*it)->getName() != "TimelineSettings")
            {
                ofRemoveListener((*it)->newGUIEvent,this,&CloudsVisualSystem::guiAllEvents);
            }
        }
    }
}

void CloudsVisualSystem::setTimelineTrackCreation(bool state)
{
    bEnableTimelineTrackCreation = state;
    if(bEnableTimelineTrackCreation)
    {
        bEnableTimeline = false;
        for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
        {
            if((*it)->getName() != "TimelineSettings")
            {
                ofAddListener((*it)->newGUIEvent,this,&CloudsVisualSystem::guiAllEvents);
            }
        }
    }
    else
    {
        for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
        {
            if((*it)->getName() != "TimelineSettings")
            {
                ofRemoveListener((*it)->newGUIEvent,this,&CloudsVisualSystem::guiAllEvents);
            }
        }
    }
}

void CloudsVisualSystem::guiAllEvents(ofxUIEventArgs &e)
{
    //All GUIS except for the Timeline UI will send events to this function
    if(bEnableTimelineTrackCreation)
    {
        bindWidgetToTimeline(e.widget);
        setTimelineTrackCreation(false);
    }
    
    if(bDeleteTimelineTrack)
    {
        unBindWidgetFromTimeline(e.widget);
        setTimelineTrackDeletion(false);
    }
}

void CloudsVisualSystem::bindWidgetToTimeline(ofxUIWidget* widget)
{
    string parentName = ((ofxUISuperCanvas *) widget->getCanvasParent())->getCanvasTitle()->getLabel();
    timeline->addPage(parentName, true);
    
    vector<ofxTLPage *> pages = timeline->getPages();
    
    ofxTLPage * currentPage = NULL;
    for(vector<ofxTLPage *>::iterator it = pages.begin(); it != pages.end(); ++it)
    {
        if((*it)->getName() == parentName)
        {
            currentPage = (*it);
        }
    }
	
    if(currentPage != NULL)
    {
        if(currentPage->getTrack(widget->getName()) != NULL)
        {
            return;
        }
    }
    
    switch (widget->getKind())
    {
        case OFX_UI_WIDGET_BUTTON:
        case OFX_UI_WIDGET_LABELBUTTON:
        case OFX_UI_WIDGET_IMAGEBUTTON:
        case OFX_UI_WIDGET_MULTIIMAGEBUTTON:
        {
            ofxUIButton *b = (ofxUIButton *) widget;
            tlButtonMap[timeline->addBangs(widget->getName())] = b;
        }
            break;
            
        case OFX_UI_WIDGET_TOGGLE:
        case OFX_UI_WIDGET_LABELTOGGLE:
        case OFX_UI_WIDGET_IMAGETOGGLE:
        case OFX_UI_WIDGET_MULTIIMAGETOGGLE:
        {
            ofxUIToggle *t = (ofxUIToggle *) widget;
            tlToggleMap[t] = timeline->addSwitches(widget->getName());
        }
            break;
            
        case OFX_UI_WIDGET_NUMBERDIALER:
        {
            ofxUINumberDialer *nd = (ofxUINumberDialer *) widget;
            tlDialerMap[nd] = timeline->addCurves(widget->getName(), ofRange(nd->getMin(), nd->getMax()), nd->getValue());
        }
            break;
            
        case OFX_UI_WIDGET_BILABELSLIDER:
        case OFX_UI_WIDGET_CIRCLESLIDER:
        case OFX_UI_WIDGET_MULTIIMAGESLIDER_H:
        case OFX_UI_WIDGET_MULTIIMAGESLIDER_V:
        case OFX_UI_WIDGET_IMAGESLIDER_H:
        case OFX_UI_WIDGET_IMAGESLIDER_V:
        case OFX_UI_WIDGET_ROTARYSLIDER:
        case OFX_UI_WIDGET_MINIMALSLIDER:
        case OFX_UI_WIDGET_SLIDER_H:
        case OFX_UI_WIDGET_SLIDER_V:
        {
            ofxUISlider *s = (ofxUISlider *) widget;
            tlSliderMap[s] = timeline->addCurves(widget->getName(), ofRange(s->getMin(), s->getMax()), s->getValue());
        }
            break;
        default:
            break;
    }
}

void CloudsVisualSystem::unBindWidgetFromTimeline(ofxUIWidget* widget)
{
    string parentName = ((ofxUISuperCanvas *) widget->getCanvasParent())->getCanvasTitle()->getLabel();
    timeline->setCurrentPage(parentName);
    
    if(!timeline->hasTrack(widget->getName()))
    {
        return;
    }
    
    
    
    switch (widget->getKind())
    {
        case OFX_UI_WIDGET_BUTTON:
        case OFX_UI_WIDGET_LABELBUTTON:
        case OFX_UI_WIDGET_IMAGEBUTTON:
        case OFX_UI_WIDGET_MULTIIMAGEBUTTON:
        {
            ofxTLBangs *track = (ofxTLBangs *) timeline->getTrack(widget->getName());
            map<ofxTLBangs *, ofxUIButton *>::iterator it = tlButtonMap.find(track);
            
            if(it != tlButtonMap.end())
            {
                timeline->removeTrack(it->first);
                tlButtonMap.erase(it);
            }
        }
            break;
            
        case OFX_UI_WIDGET_TOGGLE:
        case OFX_UI_WIDGET_LABELTOGGLE:
        case OFX_UI_WIDGET_IMAGETOGGLE:
        case OFX_UI_WIDGET_MULTIIMAGETOGGLE:
        {
            ofxUIToggle *t = (ofxUIToggle *) widget;
            map<ofxUIToggle *, ofxTLSwitches *>::iterator it = tlToggleMap.find(t);
            
            if(it != tlToggleMap.end())
            {
                timeline->removeTrack(it->second);
                tlToggleMap.erase(it);
            }
        }
            break;
            
        case OFX_UI_WIDGET_NUMBERDIALER:
        {
            ofxUINumberDialer *nd = (ofxUINumberDialer *) widget;
            map<ofxUINumberDialer *, ofxTLCurves *>::iterator it = tlDialerMap.find(nd);
            if(it != tlDialerMap.end())
            {
                timeline->removeTrack(it->second);
                tlDialerMap.erase(it);
            }
        }
            break;
            
        case OFX_UI_WIDGET_BILABELSLIDER:
        case OFX_UI_WIDGET_CIRCLESLIDER:
        case OFX_UI_WIDGET_MULTIIMAGESLIDER_H:
        case OFX_UI_WIDGET_MULTIIMAGESLIDER_V:
        case OFX_UI_WIDGET_IMAGESLIDER_H:
        case OFX_UI_WIDGET_IMAGESLIDER_V:
        case OFX_UI_WIDGET_ROTARYSLIDER:
        case OFX_UI_WIDGET_MINIMALSLIDER:
        case OFX_UI_WIDGET_SLIDER_H:
        case OFX_UI_WIDGET_SLIDER_V:
        {
            ofxUISlider *s = (ofxUISlider *) widget;
            map<ofxUISlider *, ofxTLCurves *>::iterator it = tlSliderMap.find(s);
            if(it != tlSliderMap.end())
            {
                timeline->removeTrack(it->second);
                tlSliderMap.erase(it);
            }
        }
            break;
        default:
            break;
    }
}

void CloudsVisualSystem::updateTimelineUIParams()
{
    for(map<ofxUIToggle*, ofxTLSwitches*>::iterator it = tlToggleMap.begin(); it != tlToggleMap.end(); ++it)
    {
        ofxUIToggle *t = it->first;
        ofxTLSwitches *tls = it->second;
        if(tls->isOn() != t->getValue())
        {
            t->setValue(tls->isOn());
            t->triggerSelf();
        }
    }
    
    for(map<ofxUISlider*, ofxTLCurves*>::iterator it = tlSliderMap.begin(); it != tlSliderMap.end(); ++it)
    {
        ofxUISlider *s = it->first;
        ofxTLCurves *tlc = it->second;
        s->setValue(tlc->getValue());
        s->triggerSelf();
    }
    
    for(map<ofxUINumberDialer*, ofxTLCurves*>::iterator it = tlDialerMap.begin(); it != tlDialerMap.end(); ++it)
    {
        ofxUINumberDialer *nd = it->first;
        ofxTLCurves *tlc = it->second;
        nd->setValue(tlc->getValue());
        nd->triggerSelf();
    }
}

void CloudsVisualSystem::saveTimelineUIMappings(string path)
{
    if(ofFile::doesFileExist(path))
    {
		//        cout << "DELETING OLD MAPPING FILE" << endl;
        ofFile::removeFile(path);
    }
	//    cout << "TIMELINE UI MAPPER SAVING" << endl;
    ofxXmlSettings *XML = new ofxXmlSettings(path);
    XML->clear();
    
    int mapIndex = XML->addTag("Map");
    XML->pushTag("Map", mapIndex);
    
    int bangsIndex = XML->addTag("Bangs");
    if(XML->pushTag("Bangs", bangsIndex))
    {
        for(map<ofxTLBangs*, ofxUIButton*>::iterator it = tlButtonMap.begin(); it != tlButtonMap.end(); ++it)
        {
            int index = XML->addTag("Mapping");
            if(XML->pushTag("Mapping", index))
            {
                int wIndex = XML->addTag("Widget");
                if(XML->pushTag("Widget", wIndex))
                {
                    XML->setValue("WidgetName", it->second->getName(), 0);
                    XML->setValue("WidgetID", it->second->getID(), 0);
                    XML->setValue("WidgetCanvasParent", it->second->getCanvasParent()->getName(), 0);
                    XML->popTag();
                }
                int tlIndex = XML->addTag("Track");
                if(XML->pushTag("Track", tlIndex))
                {
                    XML->popTag();
                }
                XML->popTag();
            }
        }
        XML->popTag();
    }
    
    int switchesIndex = XML->addTag("Switches");
    if(XML->pushTag("Switches", switchesIndex))
    {
        for(map<ofxUIToggle*, ofxTLSwitches*>::iterator it = tlToggleMap.begin(); it != tlToggleMap.end(); ++it)
        {
            int index = XML->addTag("Mapping");
            if(XML->pushTag("Mapping", index))
            {
                int wIndex = XML->addTag("Widget");
                if(XML->pushTag("Widget", wIndex))
                {
                    XML->setValue("WidgetName", it->first->getName(), 0);
                    XML->setValue("WidgetID", it->first->getID(), 0);
                    XML->setValue("WidgetCanvasParent", it->first->getCanvasParent()->getName(), 0);
                    XML->popTag();
                }
                int tlIndex = XML->addTag("Track");
                if(XML->pushTag("Track", tlIndex))
                {
                    XML->popTag();
                }
                XML->popTag();
            }
        }
        XML->popTag();
    }
    
    int sliderCurvesIndex = XML->addTag("SliderCurves");
    if(XML->pushTag("SliderCurves", sliderCurvesIndex))
    {
        for(map<ofxUISlider*, ofxTLCurves*>::iterator it = tlSliderMap.begin(); it != tlSliderMap.end(); ++it)
        {
            int index = XML->addTag("Mapping");
            if(XML->pushTag("Mapping", index))
            {
                int wIndex = XML->addTag("Widget");
                if(XML->pushTag("Widget", wIndex))
                {
                    XML->setValue("WidgetName", it->first->getName(), 0);
                    XML->setValue("WidgetID", it->first->getID(), 0);
                    XML->setValue("WidgetCanvasParent", it->first->getCanvasParent()->getName(), 0);
                    XML->popTag();
                }
                int tlIndex = XML->addTag("Track");
                if(XML->pushTag("Track", tlIndex))
                {
                    XML->popTag();
                }
                XML->popTag();
            }
        }
        XML->popTag();
    }
    
    int numberDialerCurvesIndex = XML->addTag("NumberDialerCurves");
    if(XML->pushTag("NumberDialerCurves", numberDialerCurvesIndex))
    {
        for(map<ofxUINumberDialer*, ofxTLCurves*>::iterator it = tlDialerMap.begin(); it != tlDialerMap.end(); ++it)
        {
            int index = XML->addTag("Mapping");
            if(XML->pushTag("Mapping", index))
            {
                int wIndex = XML->addTag("Widget");
                if(XML->pushTag("Widget", wIndex))
                {
                    XML->setValue("WidgetName", it->first->getName(), 0);
                    XML->setValue("WidgetID", it->first->getID(), 0);
                    XML->setValue("WidgetCanvasParent", it->first->getCanvasParent()->getName(), 0);
                    XML->popTag();
                }
                int tlIndex = XML->addTag("Track");
                if(XML->pushTag("Track", tlIndex))
                {
                    XML->popTag();
                }
                XML->popTag();
            }
        }
        XML->popTag();
    }
    
    XML->popTag();
    XML->saveFile(path);
    delete XML;
}

void CloudsVisualSystem::loadTimelineUIMappings(string path)
{
    tlButtonMap.clear();
    tlToggleMap.clear();
    tlSliderMap.clear();
    tlDialerMap.clear();
    
	//    cout << "LOADING TIMELINE UI MAPPINGS" << endl;
    ofxXmlSettings *XML = new ofxXmlSettings();
    XML->loadFile(path);
    if(XML->tagExists("Map",0) && XML->pushTag("Map", 0))
    {
        if(XML->pushTag("Bangs", 0))
        {
            int widgetTags = XML->getNumTags("Mapping");
            for(int i = 0; i < widgetTags; i++)
            {
                if(XML->pushTag("Mapping", i))
                {
                    if(XML->pushTag("Widget", 0))
                    {
                        string widgetname = XML->getValue("WidgetName", "NULL", 0);
                        int widgetID = XML->getValue("WidgetID", -1, 0);
                        string widgetCanvasParent = XML->getValue("WidgetCanvasParent", "NULL", 0);
                        map<string, ofxUICanvas *>::iterator it = guimap.find(widgetCanvasParent);
                        if(it != guimap.end())
                        {
                            ofxUIWidget *w = it->second->getWidget(widgetname, widgetID);
                            if(w != NULL)
                            {
                                bindWidgetToTimeline(w);
                            }
                        }
                        XML->popTag();
                    }
                    XML->popTag();
                }
            }
            XML->popTag();
        }
        
        if(XML->pushTag("Switches", 0))
        {
            int widgetTags = XML->getNumTags("Mapping");
            for(int i = 0; i < widgetTags; i++)
            {
                if(XML->pushTag("Mapping", i))
                {
                    if(XML->pushTag("Widget", 0))
                    {
                        string widgetname = XML->getValue("WidgetName", "NULL", 0);
                        int widgetID = XML->getValue("WidgetID", -1, 0);
                        string widgetCanvasParent = XML->getValue("WidgetCanvasParent", "NULL", 0);
                        map<string, ofxUICanvas *>::iterator it = guimap.find(widgetCanvasParent);
                        if(it != guimap.end())
                        {
                            ofxUIWidget *w = it->second->getWidget(widgetname, widgetID);
                            if(w != NULL)
                            {
                                bindWidgetToTimeline(w);
                            }
                        }
                        XML->popTag();
                    }
                    XML->popTag();
                }
            }
            XML->popTag();
        }
        
        if(XML->pushTag("SliderCurves", 0))
        {
            int widgetTags = XML->getNumTags("Mapping");
            for(int i = 0; i < widgetTags; i++)
            {
                if(XML->pushTag("Mapping", i))
                {
                    if(XML->pushTag("Widget", 0))
                    {
                        string widgetname = XML->getValue("WidgetName", "NULL", 0);
                        int widgetID = XML->getValue("WidgetID", -1, 0);
                        string widgetCanvasParent = XML->getValue("WidgetCanvasParent", "NULL", 0);
                        map<string, ofxUICanvas *>::iterator it = guimap.find(widgetCanvasParent);
                        if(it != guimap.end())
                        {
                            ofxUIWidget *w = it->second->getWidget(widgetname, widgetID);
                            if(w != NULL)
                            {
                                bindWidgetToTimeline(w);
                            }
                        }
                        XML->popTag();
                    }
                    XML->popTag();
                }
            }
            XML->popTag();
        }
        
        if(XML->pushTag("NumberDialerCurves", 0))
        {
            int widgetTags = XML->getNumTags("Mapping");
            for(int i = 0; i < widgetTags; i++)
            {
                if(XML->pushTag("Mapping", i))
                {
                    if(XML->pushTag("Widget", 0))
                    {
                        string widgetname = XML->getValue("WidgetName", "NULL", 0);
                        int widgetID = XML->getValue("WidgetID", -1, 0);
                        string widgetCanvasParent = XML->getValue("WidgetCanvasParent", "NULL", 0);
                        map<string, ofxUICanvas *>::iterator it = guimap.find(widgetCanvasParent);
                        if(it != guimap.end())
                        {
                            ofxUIWidget *w = it->second->getWidget(widgetname, widgetID);
                            if(w != NULL)
                            {
                                bindWidgetToTimeline(w);
                            }
                        }
                        XML->popTag();
                    }
                    XML->popTag();
                }
            }
            XML->popTag();
        }
        XML->popTag();
    }
}

void CloudsVisualSystem::lightsBegin()
{
    ofSetSmoothLighting(bSmoothLighting);
    if(bEnableLights)
    {
        for(map<string, ofxLight *>::iterator it = lights.begin(); it != lights.end(); ++it)
        {
            ofEnableLighting();
            it->second->enable();
        }
    }
}

void CloudsVisualSystem::lightsEnd()
{
    if(!bEnableLights)
    {
        ofDisableLighting();
        for(map<string, ofxLight *>::iterator it = lights.begin(); it != lights.end(); ++it)
        {
            it->second->disable();
        }
    }
}

void CloudsVisualSystem::loadGUIS()
{
    for(int i = 0; i < guis.size(); i++)
    {
        guis[i]->loadSettings(getVisualSystemDataPath()+"Working/"+getSystemName()+guis[i]->getName()+".xml");
    }
    cam.reset();
    ofxLoadCamera(cam, getVisualSystemDataPath()+"Working/"+"ofEasyCamSettings");
    resetTimeline();
    loadTimelineUIMappings(getVisualSystemDataPath()+"Working/"+getSystemName()+"UITimelineMappings.xml");
    timeline->loadTracksFromFolder(getVisualSystemDataPath()+"Working/Timeline/");
}

void CloudsVisualSystem::saveGUIS()
{
    for(int i = 0; i < guis.size(); i++)
    {
        guis[i]->saveSettings(getVisualSystemDataPath()+"Working/"+getSystemName()+guis[i]->getName()+".xml");
    }
    ofxSaveCamera(cam, getVisualSystemDataPath()+"Working/"+"ofEasyCamSettings");
    
    saveTimelineUIMappings(getVisualSystemDataPath()+"Working/"+getSystemName()+"UITimelineMappings.xml");
    
    timeline->saveTracksToFolder(getVisualSystemDataPath()+"Working/Timeline/");
}

void CloudsVisualSystem::loadPresetGUISFromName(string presetName){
	loadPresetGUISFromPath(getVisualSystemDataPath() + presetName);
}

void CloudsVisualSystem::loadPresetGUISFromPath(string presetPath)
{
	
	cout << "Loading preset data from " << presetPath << endl;
	
    for(int i = 0; i < guis.size(); i++)
    {
        guis[i]->loadSettings(presetPath+"/"+getSystemName()+guis[i]->getName()+".xml");
    }
    cam.reset();
    ofxLoadCamera(cam, presetPath+"/"+"ofEasyCamSettings");
    
    resetTimeline();
	
    loadTimelineUIMappings(presetPath+"/"+getSystemName()+"UITimelineMappings.xml");
    timeline->loadTracksFromFolder(presetPath+"/Timeline/");
    timeline->saveTracksToFolder(getVisualSystemDataPath()+"Working/Timeline/");
	
	selfPresetLoaded(presetPath);
}

void CloudsVisualSystem::savePresetGUIS(string presetName)
{
    ofDirectory dir;
    string presetDirectory = getVisualSystemDataPath()+presetName+"/";
    if(!dir.doesDirectoryExist(presetDirectory))
    {
        dir.createDirectory(presetDirectory);
        presetRadio->addToggle(presetGui->addToggle(presetName, true));
        presetGui->autoSizeToFitWidgets();
    }
    
    for(int i = 0; i < guis.size(); i++)
    {
        guis[i]->saveSettings(presetDirectory+getSystemName()+guis[i]->getName()+".xml");
    }
    ofxSaveCamera(cam, getVisualSystemDataPath()+presetName+"/"+"ofEasyCamSettings");
    saveTimelineUIMappings(getVisualSystemDataPath()+presetName+"/"+getSystemName()+"UITimelineMappings.xml");
    timeline->saveTracksToFolder(getVisualSystemDataPath()+presetName+"/Timeline/");
    timeline->saveTracksToFolder(getVisualSystemDataPath()+"Working/Timeline/");
}

void CloudsVisualSystem::deleteGUIS()
{
    for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
    {
        ofxUICanvas *g = (*it);
        delete g;
    }
    guis.clear();
}

void CloudsVisualSystem::showGUIS()
{
    for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
    {
        (*it)->enable();
    }
}

void CloudsVisualSystem::hideGUIS()
{
    for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
    {
        (*it)->disable();
    }
}

void CloudsVisualSystem::toggleGUIS()
{
    for(vector<ofxUISuperCanvas *>::iterator it = guis.begin(); it != guis.end(); ++it)
    {
        (*it)->toggleVisible();
    }
}

void CloudsVisualSystem::toggleGuiAndPosition(ofxUISuperCanvas *g)
{
    if(g->isMinified())
    {
        g->setMinified(false);
        g->setPosition(ofGetMouseX(), ofGetMouseY());
    }
    else
    {
        g->setMinified(true);
    }
}

void CloudsVisualSystem::setCurrentCamera(ofCamera& swappedInCam){
	currentCamera = &swappedInCam;
}

void CloudsVisualSystem::drawDebug()
{
    if(bDebug)
    {
		ofPushStyle();
        float color = 255-bgBri->getPos();
        ofEnableBlendMode(OF_BLENDMODE_ALPHA);
        drawGrid(-debugGridSize,-debugGridSize,debugGridSize*2,debugGridSize*2, color);
        drawAxis(debugGridSize, color);
        selfDrawDebug();
		ofPopStyle();
    }
}

void CloudsVisualSystem::drawAxis(float size, float color)
{
    ofSetColor(color, 100);
    ofLine(-size, 0, 0, size, 0, 0);
    ofLine(0, -size, 0, 0, size, 0);
    ofLine(0, 0, -size, 0, 0, size);
}

void CloudsVisualSystem::drawGrid(float x, float y, float w, float h, float color)
{
    ofNoFill();
    ofSetColor(color, 25);
    ofRect(x, y, w, h);
    
    float md = MAX(w,h);
    ofSetColor(color, 50);
    for(float i = 0; i <= w; i+=md*.025)
    {
        ofLine(x+i, y, x+i, y+h);
    }
    for(float j = 0; j <= h; j+=md*.025)
    {
        ofLine(x, y+j, x+w, y+j);
    }
}

void CloudsVisualSystem::billBoard(ofVec3f globalCamPosition, ofVec3f globelObjectPosition)
{
    ofVec3f objectLookAt = ofVec3f(0,0,1);
    ofVec3f objToCam = globalCamPosition - globelObjectPosition;
    objToCam.normalize();
    float theta = objectLookAt.angle(objToCam);
    ofVec3f axisOfRotation = objToCam.crossed(objectLookAt);
    axisOfRotation.normalize();
    glRotatef(-theta, axisOfRotation.x, axisOfRotation.y, axisOfRotation.z);
}

void CloudsVisualSystem::drawTexturedQuad()
{
    glBegin (GL_QUADS);
    
    glTexCoord2f (0.0, 0.0);
    glVertex3f (0.0, 0.0, 0.0);
    
    glTexCoord2f (ofGetWidth(), 0.0);
    glVertex3f (ofGetWidth(), 0.0, 0.0);
    
    
    glTexCoord2f (ofGetWidth(), ofGetHeight());
    glVertex3f (ofGetWidth(), ofGetHeight(), 0.0);
    
    glTexCoord2f (0.0, ofGetHeight());
    glVertex3f (0.0, ofGetHeight(), 0.0);
    
    glEnd ();
}

void CloudsVisualSystem::drawNormalizedTexturedQuad()
{
    glBegin (GL_QUADS);
    
    glTexCoord2f (0.0, 0.0);
    glVertex3f (0.0, 0.0, 0.0);
    
    glTexCoord2f (1.0, 0.0);
    glVertex3f (ofGetWidth(), 0.0, 0.0);
    
    
    glTexCoord2f (1.0, 1.0);
    glVertex3f (ofGetWidth(), ofGetHeight(), 0.0);
    
    glTexCoord2f (0.0, 1.0);
    glVertex3f (0.0, ofGetHeight(), 0.0);
    
    glEnd ();
}

void CloudsVisualSystem::drawBackground()
{
	ofPushStyle();
	//	glPushAttrib(GL_ALL_ATTRIB_BITS);
	
	ofEnableAlphaBlending();
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_LIGHTING);
	
    if(bClearBackground)
	{
		if(gradientMode == OF_GRADIENT_CIRCULAR)
		{
			
			//  TEMPORAL FIX
			//
			//		cout << "drawing bckground color " << *bgColor << " " << *bgColor2 << endl;
			ofSetSmoothLighting(true);
			ofBackgroundGradient(*bgColor, *bgColor2, OF_GRADIENT_CIRCULAR);
			
			//  Sorry Reza this is a quick and durty fix
			//
			//        ofPushMatrix();
			//        if(camFOV > 60)
			//        {
			//            ofBackground(*bgColor2);
			//        }
			//        billBoard(cam.getGlobalPosition(), ofVec3f(0,0,0));
			//        ofDisableLighting();
			//        ofSetSmoothLighting(true);
			//        glNormal3f(0,0,1);
			//        ofLayerGradient(*bgColor, *bgColor2);
			//        ofPopMatrix();
			
		}
		else
		{
			ofSetSmoothLighting(false);
			ofBackground(*bgColor);
		}
	}

	//	glPopAttrib();
	
	
	ofPopStyle();
	

	ofPushStyle();
	ofPushMatrix();
	ofTranslate(0, ofGetHeight());
	ofScale(1,-1,1);
	selfDrawBackground();
	ofPopMatrix();
	ofPopStyle();
}

void CloudsVisualSystem::ofLayerGradient(const ofColor& start, const ofColor& end)
{
    float w = cam.getDistance()*bgAspectRatio;
    float h = cam.getDistance()*bgAspectRatio;
    ofMesh mesh;
    mesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
    // this could be optimized by building a single mesh once, then copying
    // it and just adding the colors whenever the function is called.
    ofVec2f center(0.0,0.0);
    mesh.addVertex(center);
    mesh.addColor(start);
    int n = 32; // circular gradient resolution
    float angleBisector = TWO_PI / (n * 2);
    float smallRadius = ofDist(0, 0, w / 2, h / 2);
    float bigRadius = smallRadius / cos(angleBisector);
    for(int i = 0; i <= n; i++) {
        float theta = i * TWO_PI / n;
        mesh.addVertex(center + ofVec2f(sin(theta), cos(theta)) * bigRadius);
        mesh.addColor(end);
    }
    glDepthMask(false);
    mesh.draw();
    glDepthMask(true);
}

//Grab These Methods
string CloudsVisualSystem::getSystemName()
{
	return "CloudsVisualSystem";
}

void CloudsVisualSystem::selfSetup()
{
    
}

void CloudsVisualSystem::selfPresetLoaded(string presetPath)
{
	
}

void CloudsVisualSystem::selfSetupGuis()
{
    
}

void CloudsVisualSystem::selfUpdate()
{
    
}

void CloudsVisualSystem::selfDrawBackground()
{
    
}

void CloudsVisualSystem::selfDrawDebug()
{
    
}

void CloudsVisualSystem::selfSceneTransformation()
{
    
}

void CloudsVisualSystem::selfDraw()
{
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    mat->begin();
    ofSetColor(ofColor(255));
    ofFill();
    mat->end();
}

void CloudsVisualSystem::selfDrawOverlay(){
	
}

void CloudsVisualSystem::selfPostDraw(){
	
	CloudsVisualSystem::getSharedRenderTarget().draw(0,CloudsVisualSystem::getSharedRenderTarget().getHeight(),
													 CloudsVisualSystem::getSharedRenderTarget().getWidth(),
													 -CloudsVisualSystem::getSharedRenderTarget().getHeight());	
}

void CloudsVisualSystem::selfExit()
{
    
}

void CloudsVisualSystem::selfBegin()
{
    
}

void CloudsVisualSystem::selfEnd()
{
    
}

void CloudsVisualSystem::selfKeyPressed(ofKeyEventArgs & args)
{
    
}

void CloudsVisualSystem::selfKeyReleased(ofKeyEventArgs & args)
{
    
}

void CloudsVisualSystem::selfMouseDragged(ofMouseEventArgs& data)
{
    
}

void CloudsVisualSystem::selfMouseMoved(ofMouseEventArgs& data)
{
    
}

void CloudsVisualSystem::selfMousePressed(ofMouseEventArgs& data)
{
    
}

void CloudsVisualSystem::selfMouseReleased(ofMouseEventArgs& data)
{
    
}

void CloudsVisualSystem::selfSetupGui()
{
	
}

void CloudsVisualSystem::selfGuiEvent(ofxUIEventArgs &e)
{
    
}

void CloudsVisualSystem::selfSetupSystemGui()
{
    
}

void CloudsVisualSystem::guiSystemEvent(ofxUIEventArgs &e)
{
    
}

void CloudsVisualSystem::selfSetupRenderGui()
{
    
}

void CloudsVisualSystem::guiRenderEvent(ofxUIEventArgs &e)
{
    
}

void CloudsVisualSystem::selfSetupTimeline()
{
    
}

void CloudsVisualSystem::selfSetupTimelineGui()
{
    
}

void CloudsVisualSystem::selfTimelineGuiEvent(ofxUIEventArgs &e)
{
    
}