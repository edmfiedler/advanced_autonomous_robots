/***************************************************************************
 *   Copyright (C) 2005 by Christian Andersen and DTU                      *
 *   jca@oersted.dtu.dk                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "scanright.h"

#ifdef LIBRARY_OPEN_NEEDED

/**
 * This function is needed by the server to create a version of this plugin */
UFunctionBase * createFunc()
{ // create an object of this type
  /** replace 'UFunczoneobst' with your class name */
  return new scanright();
}
#endif

bool scanright::setResource(UResBase * resource, bool remove) { // load resource as provided by the server (or other plugins)
	bool result = true;

	if (resource->isA(UResPoseHist::getOdoPoseID())) { // pointer to server the resource that this plugin can provide too
		// but as there might be more plugins that can provide the same resource
		// use the provided
		if (remove)
			// the resource is unloaded, so reference must be removed
			poseHist = NULL;
		else if (poseHist != (UResPoseHist *) resource)
			// resource is new or is moved, save the new reference
			poseHist = (UResPoseHist *) resource;
		else
			// reference is not used
			result = false;
	}

	// other resource types may be needed by base function.
	result = UFunctionBase::setResource(resource, remove);
	return result;
}



///////////////////////////////////////////////////
///////////////////////////////////////////////////
///////////////////////////////////////////////////
///////////////////////////////////////////////////
// #define SMRWIDTH 0.4
bool scanright::handleCommand(UServerInMsg * msg, void * extra)
{  // handle a plugin command
  const int MRL = 500;
  char reply[MRL];
  bool ask4help;
  const int MVL = 30;
  char value[MVL];
  ULaserData * data;
  double robotwidth;
  //
  int i,j,imax;
  double r,delta;
  double minRange; // min range in meter
  // double minAngle = 0.0; // degrees
//   double d,robotwidth;
  double zone[9];
  // check for parameters - one parameter is tested for - 'help'
  ask4help = msg->tag.getAttValue("help", value, MVL);
  if (ask4help)
  { // create the reply in XML-like (html - like) format
    sendHelpStart(msg, "relocate");
    sendText("--- available zoneobst options\n");
    sendText("help            This message\n");
    sendText("fake=F          Fake some data 1=random, 2-4 a fake corridor\n");
    sendText("device=N        Laser device to use (see: SCANGET help)\n");
    sendText("see also: SCANGET and SCANSET\n");
    sendHelpDone();
  }
  else
  { // do some action and send a reply
    data = getScan(msg, (ULaserData*)extra);
    // 
    if (data->isValid())
    {
    // check if a attribute (parameter) with name width exists and if so get its value
       bool gotwidth = msg->tag.getAttValue("width", value, MVL);
       if (gotwidth) {
        	robotwidth=strtod(value, NULL);   
       }
       else {
        	robotwidth=0.26;
      }
      UPose poseAtScan = poseHist->getPoseAtTime(data->getScanTime());
      // Gets the odometry pose at the time when the laserscan was taken, poseAtScan.x poseAtScan.y poseAtScan.a (x,y,angle)
      // make analysis for closest measurement

      /*
      minRange = 1000; // long range in meters
      imax=data->getRangeCnt();
      delta=imax/9.0;
      for (j=0;j<9;j++)
	 zone[j]=minRange;
      for(j=0;j<9;j++){
      for (i = 0+(int)(j*delta); i < (int)((j+1)*delta); i++)
      { // range are stored as an integer in current units
	r = data->getRangeMeter(i);
        if (r >= 0.020)
        { // less than 20 units is a flag value for URG scanner
          if (r<zone[j])
	     zone[j]=r;
        }
      }
      }
      */
	
        imax=data->getRangeCnt();

	double r1_dist, r1_ang, r;
	// Obtain the detected furthest point from the right of the robot
	for (i=imax;i>0;i--){
		r = data->getRangeMeter(i);
		if (r>=0.02){
			if (r<=0.8){
			r1_dist = r;
			r1_ang = (-i*0.36)*3.1416/180;
			break;
			}
		}
	}

	double x1, y1;
	x1 = r1_dist*cos(r1_ang)-0.5;
	y1 = r1_dist*sin(r1_ang)-0.26;
	
/*
	double min1_dist, min1_ang;
	// Obtain the nearest point (closest corner to the robot)
	for (i=0;i<imax-1;i++){
		if (data->getRangeMeter(i)>=0.02){
			if (data->getRangeMeter(i)<data->getRangeMeter(i+1)){
				min1_dist = data->getRangeMeter(i);
				min1_ang = (90-i*0.36)*3.14/180;
				break;
			}
		}
	}
*/

      /* SMRCL reply format */
      snprintf(reply, MRL, "<laser l5=\"%g\" l6=\"%g\" l7=\"%g\" l8=\"%g\"/>\n", 
	                  r1_dist,r1_ang,x1,y1);
      // send this string as the reply to the client
      sendMsg(msg, reply);
      // save also as gloabl variable
      	var_zone->setValued(r1_dist,0);
	//var_zone->setValued(parky,1);
    }
    else
      sendWarning(msg, "No scandata available");
  }
  // return true if the function is handled with a positive result
  // used if scanpush or push has a count of positive results
  return true;
}

void scanright::createBaseVar()
{ // add also a global variable (global on laser scanner server) with latest data
  var_zone = addVarA("zone", "0 0 0 0 0 0 0 0 0", "d", "Value of each laser zone. Updated by zoneobst.");
}
