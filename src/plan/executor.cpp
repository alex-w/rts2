#include "../utilsdb/rts2devicedb.h"
#include "../utilsdb/target.h"
#include "rts2execcli.h"
#include "rts2execclidb.h"
#include "rts2devcliphot.h"

#include <vector>

class Rts2Executor;

class Rts2ConnExecutor:public Rts2DevConn
{
private:
  Rts2Executor * master;
protected:
  virtual int commandAuthorized ();
public:
    Rts2ConnExecutor (int in_sock, Rts2Executor * in_master);
};

class Rts2Executor:public Rts2DeviceDb
{
private:
  Target * currentTarget;
  Target *nextTarget;
  Target *priorityTarget;
  void queDarks ();
  void queFlats ();
  void beforeChange ();
  void doSwitch ();
  void switchTarget ();

  Rts2ValueInteger *scriptCount;	// -1 means no exposure registered (yet), > 0 means scripts in progress, 0 means all script finished
  int waitState;
    std::vector < Target * >targetsQue;
  struct ln_lnlat_posn *observer;

  int ignoreDay;
  double grb_sep_limit;
  double grb_min_sep;

  Rts2ValueInteger *acqusitionOk;
  Rts2ValueInteger *acqusitionFailed;

  int setNow (Target * newTarget);

  void queTarget (Target * in_target);
  void updateScriptCount ();

  Rts2ValueInteger *current_id;
  Rts2ValueInteger *current_id_sel;
  Rts2ValueInteger *current_obsid;

  Rts2ValueInteger *next_id;
  Rts2ValueInteger *priority_id;

  Rts2ValueInteger *img_id;

protected:
    virtual int processOption (int in_opt);
  virtual int reloadConfig ();
public:
    Rts2Executor (int argc, char **argv);
    virtual ~ Rts2Executor (void);
  virtual int init ();
  virtual Rts2DevConn *createConnection (int in_sock);
  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);

  virtual void postEvent (Rts2Event * event);

  virtual void deviceReady (Rts2Conn * conn);

  virtual int ready ()
  {
    return 0;
  }
  virtual int info ();

  virtual int changeMasterState (int new_state);

  int setNext (int nextId);
  int setNow (int nextId);
  int setGrb (int grbId);
  int setShower ();

  int end ()
  {
    maskState (EXEC_MASK_END, EXEC_END);
    return 0;
  }

  int stop ();
};

int
Rts2ConnExecutor::commandAuthorized ()
{
  int tar_id;
  if (isCommand ("grb"))
    {
      // change observation if we are to far from GRB position..
      if (paramNextInteger (&tar_id) || !paramEnd ())
	return -2;
      return master->setGrb (tar_id);
    }
  else if (isCommand ("shower"))
    {
      if (!paramEnd ())
	return -2;
      return master->setShower ();
    }
  else if (isCommand ("now"))
    {
      // change observation imediatelly - in case of burst etc..
      if (paramNextInteger (&tar_id) || !paramEnd ())
	return -2;
      return master->setNow (tar_id);
    }
  else if (isCommand ("next"))
    {
      if (paramNextInteger (&tar_id) || !paramEnd ())
	return -2;
      return master->setNext (tar_id);
    }
  else if (isCommand ("end"))
    {
      if (!paramEnd ())
	return -2;
      return master->end ();
    }
  else if (isCommand ("stop"))
    {
      if (!paramEnd ())
	return -2;
      return master->stop ();
    }
  return Rts2DevConn::commandAuthorized ();
}

Rts2ConnExecutor::Rts2ConnExecutor (int in_sock, Rts2Executor * in_master):Rts2DevConn (in_sock,
	     in_master)
{
  master = in_master;
}

Rts2Executor::Rts2Executor (int in_argc, char **in_argv):
Rts2DeviceDb (in_argc, in_argv, DEVICE_TYPE_EXECUTOR, "EXEC")
{
  currentTarget = NULL;
  nextTarget = NULL;
  priorityTarget = NULL;
  createValue (scriptCount, "script_count", "number of running scripts",
	       false);
  scriptCount->setValueInteger (-1);

  addOption ('I', "ignore_day", 0, "observe even during daytime");
  ignoreDay = 0;

  grb_sep_limit = -1;
  grb_min_sep = 0;

  waitState = 0;

  createValue (acqusitionOk, "acqusition_ok",
	       "number of acqusitions completed sucesfully", false);
  acqusitionOk->setValueInteger (0);

  createValue (acqusitionFailed, "acqusition_failed",
	       "number of acqusitions which failed", false);
  acqusitionFailed->setValueInteger (0);

  createValue (current_id, "current", "ID of current target", false);
  createValue (current_id_sel, "current_sel",
	       "ID of currently selected target", false);
  createValue (current_obsid, "obsid", "ID of observation", false);

  createValue (next_id, "next", "ID of next target", false);

  createValue (priority_id, "priority_target",
	       "ID of priority target (should be NULL in most cases", false);

  createValue (img_id, "img_id", "ID of current image", false);
}

Rts2Executor::~Rts2Executor (void)
{
  postEvent (new Rts2Event (EVENT_KILL_ALL));
  delete currentTarget;
  delete nextTarget;
}

int
Rts2Executor::processOption (int in_opt)
{
  switch (in_opt)
    {
    case 'I':
      ignoreDay = 1;
      break;
    default:
      return Rts2DeviceDb::processOption (in_opt);
    }
  return 0;
}

int
Rts2Executor::reloadConfig ()
{
  int ret;
  Rts2Config *config;
  ret = Rts2DeviceDb::reloadConfig ();
  if (ret)
    return ret;
  config = Rts2Config::instance ();
  observer = config->getObserver ();
  config->getDouble ("grbd", "seplimit", grb_sep_limit);
  config->getDouble ("grbd", "minsep", grb_min_sep);
  return 0;
}

int
Rts2Executor::init ()
{
  int ret;
  ret = Rts2DeviceDb::init ();
  if (ret)
    return ret;
  // set priority..
  getCentraldConn ()->queCommand (new Rts2Command (this, "priority 20"));
  return 0;
}

Rts2DevConn *
Rts2Executor::createConnection (int in_sock)
{
  return new Rts2ConnExecutor (in_sock, this);
}

Rts2DevClient *
Rts2Executor::createOtherType (Rts2Conn * conn, int other_device_type)
{
  switch (other_device_type)
    {
    case DEVICE_TYPE_MOUNT:
      return new Rts2DevClientTelescopeExec (conn);
    case DEVICE_TYPE_CCD:
      return new Rts2DevClientCameraExecDb (conn);
    case DEVICE_TYPE_FOCUS:
      return new Rts2DevClientFocusImage (conn);
    case DEVICE_TYPE_PHOT:
      return new Rts2DevClientPhotExec (conn);
    case DEVICE_TYPE_MIRROR:
      return new Rts2DevClientMirrorExec (conn);
    case DEVICE_TYPE_DOME:
    case DEVICE_TYPE_SENSOR:
      return new Rts2DevClientWriteImage (conn);
    default:
      return Rts2DeviceDb::createOtherType (conn, other_device_type);
    }
}

void
Rts2Executor::postEvent (Rts2Event * event)
{
  switch (event->getType ())
    {
    case EVENT_SLEW_TO_TARGET:
      maskState (EXEC_STATE_MASK, EXEC_MOVE);
      break;
    case EVENT_OBSERVE:
      // we can get EVENT_OBSERVE in case of continues observation
    case EVENT_SCRIPT_STARTED:
      maskState (EXEC_STATE_MASK, EXEC_OBSERVE);
      break;
    case EVENT_ACQUIRE_START:
      maskState (EXEC_STATE_MASK, EXEC_ACQUIRE);
      break;
    case EVENT_ACQUIRE_WAIT:
      maskState (EXEC_STATE_MASK, EXEC_ACQUIRE_WAIT);
      break;
    case EVENT_ACQUSITION_END:
      // we receive event before any connection - connections
      // receive it from Rts2Device.
      // So we can safely change target status here, and it will
      // propagate to devices connections
      maskState (EXEC_STATE_MASK, EXEC_OBSERVE);
      switch (*(int *) event->getArg ())
	{
	case NEXT_COMMAND_PRECISION_OK:
	  if (currentTarget)
	    {
	      logStream (MESSAGE_DEBUG) << "NEXT_COMMAND_PRECISION_OK " <<
		currentTarget->getObsTargetID () << sendLog;
	      currentTarget->acqusitionEnd ();
	    }
	  maskState (EXEC_MASK_ACQ, EXEC_ACQ_OK);
	  break;
	case -5:
	case NEXT_COMMAND_PRECISION_FAILED:
	  if (currentTarget)
	    currentTarget->acqusitionFailed ();
	  maskState (EXEC_MASK_ACQ, EXEC_ACQ_FAILED);
	  break;
	}
      break;
    case EVENT_LAST_READOUT:
      updateScriptCount ();
      // that was last script running
      if (scriptCount->getValueInteger () == 0)
	maskState (EXEC_STATE_MASK, EXEC_LASTREAD);
      break;
    case EVENT_SCRIPT_ENDED:
      updateScriptCount ();
      if (currentTarget)
	{
	  if (scriptCount->getValueInteger () == 0
	      && currentTarget->observationStarted ())
	    {
	      maskState (EXEC_STATE_MASK, EXEC_IDLE);
	      switchTarget ();
	    }
	  // scriptCount is not 0, but we hit continues target..
	  else if (currentTarget->isContinues () == 1
		   && (nextTarget == NULL
		       || nextTarget->getTargetID () ==
		       currentTarget->getTargetID ()))
	    {
	      // wait, if we are in stop..don't que it again..
	      if ((getState () & EXEC_MASK_END) != EXEC_END)
		event->setArg ((void *) currentTarget);
	      // that will eventually hit devclient which post that message, which
	      // will set currentTarget to this value and handle it same way as EVENT_OBSERVE,
	      // which is exactly what we want
	    }
	}
      else
	{
	  if (scriptCount->getValueInteger () == 0)
	    switchTarget ();
	}
      break;
    case EVENT_MOVE_OK:
      if (waitState)
	{
	  postEvent (new Rts2Event (EVENT_CLEAR_WAIT));
	  break;
	}
      postEvent (new Rts2Event (EVENT_OBSERVE));
      break;
    case EVENT_MOVE_FAILED:
      if (*((int *) event->getArg ()) == DEVICE_ERROR_KILL && priorityTarget)
	{
	  // we are free to start new hig-priority observation
	  if (currentTarget)
	    queTarget (currentTarget);
	  currentTarget = priorityTarget;
	  priorityTarget = NULL;
	  postEvent (new
		     Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
	  postEvent (new Rts2Event (EVENT_SLEW_TO_TARGET));
	  infoAll ();
	  break;
	}
      postEvent (new Rts2Event (EVENT_STOP_OBSERVATION));
      if (waitState)
	{
	  postEvent (new Rts2Event (EVENT_CLEAR_WAIT));
	}
      else if (currentTarget)
	{
	  // get us lover priority to prevent moves to such dangerous
	  // position
	  currentTarget->changePriority (-100,
					 ln_get_julian_from_sys () +
					 12 * (1.0 / 1440.0));
	}
      updateScriptCount ();
      if (scriptCount->getValueInteger () == 0)
	switchTarget ();
      break;
    case EVENT_ENTER_WAIT:
      waitState = 1;
      break;
    case EVENT_CLEAR_WAIT:
      waitState = 0;
      break;
    case EVENT_GET_ACQUIRE_STATE:
      *((int *) event->getArg ()) =
	(currentTarget) ? currentTarget->getAcquired () : -2;
      break;
    }
  Rts2Device::postEvent (event);
}

void
Rts2Executor::deviceReady (Rts2Conn * conn)
{
  if (currentTarget)
    conn->
      postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
}

int
Rts2Executor::info ()
{
  updateScriptCount ();
  if (currentTarget)
    {
      current_id->setValueInteger (currentTarget->getObsTargetID ());
      current_id_sel->setValueInteger (currentTarget->getTargetID ());
      current_obsid->setValueInteger (currentTarget->getObsId ());
      img_id->setValueInteger (currentTarget->getCurrImgId ());
    }
  else
    {
      current_id->setValueInteger (-1);
      current_id_sel->setValueInteger (-1);
      current_obsid->setValueInteger (-1);
      img_id->setValueInteger (-1);
    }
  if (nextTarget)
    {
      next_id->setValueInteger (nextTarget->getTargetID ());
    }
  else
    {
      next_id->setValueInteger (-1);
    }
  if (priorityTarget)
    {
      priority_id->setValueInteger (priorityTarget->getTargetID ());
    }
  else
    {
      priority_id->setValueInteger (-1);
    }

  return Rts2DeviceDb::info ();
}

int
Rts2Executor::changeMasterState (int new_state)
{
  if (ignoreDay)
    return Rts2DeviceDb::changeMasterState (new_state);

  switch (new_state)
    {
    case SERVERD_EVENING:
    case SERVERD_DAWN:
    case SERVERD_NIGHT:
    case SERVERD_DUSK:
      // unblock stop state
      if ((getState () & EXEC_MASK_END) == EXEC_END)
	{
	  maskState (EXEC_MASK_END, EXEC_NOT_END);
	}
      if (!currentTarget && nextTarget)
	{
	  switchTarget ();
	}
      break;
    case (SERVERD_DAWN | SERVERD_STANDBY):
    case (SERVERD_NIGHT | SERVERD_STANDBY):
    case (SERVERD_DUSK | SERVERD_STANDBY):
      delete nextTarget;
      // next will be dark..
      nextTarget = createTarget (1, observer);
      if (!currentTarget)
	{
	  switchTarget ();
	}
      break;
    default:
      // we need to stop observation that is continuus
      // that will guarantie that in isContinues call, we will not que our target again
      end ();
      break;
    }
  return Rts2DeviceDb::changeMasterState (new_state);
}

int
Rts2Executor::setNext (int nextId)
{
  if (nextTarget)
    {
      if (nextTarget->getTargetID () == nextId)
	// we observe the same target..
	return 0;
      delete nextTarget;
    }
  nextTarget = createTarget (nextId, observer);
  if (!nextTarget)
    {
      // error
      return -2;
    }
  if (!currentTarget)
    switchTarget ();
  else
    infoAll ();
  return 0;
}

int
Rts2Executor::setNow (int nextId)
{
  Target *newTarget;

  if (!currentTarget)
    return setNext (nextId);

  newTarget = createTarget (nextId, observer);
  if (!newTarget)
    {
      // error..
      return -2;
    }

  return setNow (newTarget);
}

int
Rts2Executor::setNow (Target * newTarget)
{
  if (currentTarget)
    {
      currentTarget->endObservation (-1);
      queTarget (currentTarget);
    }
  currentTarget = NULL;

  if (priorityTarget)
    delete priorityTarget;	// delete old priority target

  priorityTarget = newTarget;

  // at this situation, we would like to get rid of nextTarget as
  // well
  if (nextTarget)
    {
      delete nextTarget;
      nextTarget = NULL;
    }

  postEvent (new Rts2Event (EVENT_KILL_ALL));
  queAll (new Rts2CommandKillAll (this));
  infoAll ();

  return 0;
}

int
Rts2Executor::setGrb (int grbId)
{
  Target *grbTarget;
  struct ln_hrz_posn grbHrz;
  int ret;

  // is during night and ready?
  if (!(getMasterState () == SERVERD_NIGHT
	|| getMasterState () == SERVERD_DUSK
	|| getMasterState () == SERVERD_DAWN))
    {
      logStream (MESSAGE_DEBUG) << "Rts2Executor::setGrb daylight GRB ignored"
	<< sendLog;
      return -2;
    }

  grbTarget = createTarget (grbId, observer);

  if (!grbTarget)
    {
      return -2;
    }
  ret = grbTarget->getAltAz (&grbHrz);
  // don't care if we don't get altitude from libnova..it's completly weird, but we should not
  // miss GRB:(
  if (!ret)
    {
      logStream (MESSAGE_DEBUG) << "Rts2Executor::setGrb grbHrz alt:" <<
	grbHrz.alt << sendLog;
      if (grbHrz.alt < 0)
	{
	  delete grbTarget;
	  return -2;
	}
    }
  if (!currentTarget)
    {
      // if we don't observe anything..bring us to GRB..
      if (priorityTarget)
	{
	  // it's not same..
	  ret = grbTarget->compareWithTarget (priorityTarget, grb_sep_limit);
	  if (ret == 0)
	    {
	      return setNow (grbTarget);
	    }
	  else
	    {
	      // wait till it will be properly processed
	      return 0;
	    }
	}
      delete nextTarget;
      nextTarget = grbTarget;
      switchTarget ();
      return 0;
    }
  // it's not same..
  ret = grbTarget->compareWithTarget (currentTarget, grb_sep_limit);
  if (ret == 0)
    {
      return setNow (grbTarget);
    }
  // if that's only few arcsec update, don't change
  ret = grbTarget->compareWithTarget (currentTarget, grb_min_sep);
  if (ret == 1)
    {
      return 0;
    }
  // otherwise set us as next target
  delete nextTarget;
  nextTarget = grbTarget;
  return 0;
}

int
Rts2Executor::setShower ()
{
  // is during night and ready?
  if (!(getMasterState () == SERVERD_NIGHT))
    {
      logStream (MESSAGE_DEBUG) <<
	"Rts2Executor::setShow daylight shower ignored" << sendLog;
      return -2;
    }

  return setNow (TARGET_SHOWER);
}

int
Rts2Executor::stop ()
{
  postEvent (new Rts2Event (EVENT_STOP_OBSERVATION));
  updateScriptCount ();
  if (scriptCount->getValueInteger () == 0)
    switchTarget ();
  return 0;
}

void
Rts2Executor::queDarks ()
{
  Rts2Conn *minConn = getMinConn ("que_size");
  if (!minConn)
    return;
  minConn->queCommand (new Rts2Command (this, "que_darks"));
}

void
Rts2Executor::queFlats ()
{
  Rts2Conn *minConn = getMinConn ("que_size");
  if (!minConn)
    return;
  minConn->queCommand (new Rts2Command (this, "que_flats"));
}

void
Rts2Executor::beforeChange ()
{
  // both currentTarget and nextTarget are defined
  char currType;
  char nextType;
  if (currentTarget)
    currType = currentTarget->getTargetType ();
  else
    currType = TYPE_UNKNOW;
  if (nextTarget)
    nextType = nextTarget->getTargetType ();
  else
    nextType = currType;
  if (currType == TYPE_DARK && nextType != TYPE_DARK)
    queDarks ();
  if (currType == TYPE_FLAT && nextType != TYPE_FLAT)
    queFlats ();
}

void
Rts2Executor::doSwitch ()
{
  int ret;
  int nextId;
  // we need to change current target - usefull for planner runs
  if (currentTarget && currentTarget->isContinues () == 2
      && (!nextTarget
	  || nextTarget->getTargetID () == currentTarget->getTargetID ()))
    {
      delete nextTarget;
      // create again our target..since conditions changed, we will get different target id
      nextTarget = createTarget (currentTarget->getTargetID (), observer);
    }
  // check dark and flat processing
  beforeChange ();
  if (nextTarget)
    {
      // go to post-process
      if (currentTarget)
	{
	  // nex target is defined - tested on line -5
	  nextId = nextTarget->getTargetID ();
	  ret = currentTarget->endObservation (nextId);
	  if (!(ret == 1 && nextId == currentTarget->getTargetID ()))
	    // don't que only in case nextTarget and currentTarget are
	    // same and endObservation returns 1
	    {
	      queTarget (currentTarget);
	      currentTarget = nextTarget;
	      nextTarget = NULL;
	    }
	}
      else
	{
	  currentTarget = nextTarget;
	  nextTarget = NULL;
	}
    }
  if (currentTarget)
    {
      postEvent (new Rts2Event (EVENT_SET_TARGET, (void *) currentTarget));
      postEvent (new Rts2Event (EVENT_SLEW_TO_TARGET));
    }
}

void
Rts2Executor::switchTarget ()
{
  if ((getState () & EXEC_MASK_END) == EXEC_END)
    {
      maskState (EXEC_MASK_END, EXEC_NOT_END);
      postEvent (new Rts2Event (EVENT_KILL_ALL));
      if (currentTarget)
	{
	  currentTarget->endObservation (-1);
	  queTarget (currentTarget);
	}
      currentTarget = NULL;
      if (nextTarget)
	{
	  delete nextTarget;
	  nextTarget = NULL;
	}
    }
  else if (ignoreDay)
    {
      doSwitch ();
    }
  else
    {
      // we will not observe during daytime..
      switch (getMasterState ())
	{
	case SERVERD_EVENING:
	case SERVERD_DUSK:
	case SERVERD_NIGHT:
	case SERVERD_DAWN:
	  doSwitch ();
	  break;
	case SERVERD_DUSK | SERVERD_STANDBY:
	case SERVERD_DAWN | SERVERD_STANDBY:
	  if (!currentTarget && nextTarget && nextTarget->getTargetID () == 1)
	    {
	      // switch to dark..
	      doSwitch ();
	      break;
	    }
	default:
	  if (currentTarget)
	    {
	      currentTarget->endObservation (-1);
	      queTarget (currentTarget);
	    }
	  currentTarget = NULL;
	  if (nextTarget)
	    {
	      delete nextTarget;
	      nextTarget = NULL;
	    }
	}
    }
  infoAll ();
}

void
Rts2Executor::queTarget (Target * in_target)
{
  int ret;
  // test for final acq..
  switch (in_target->getAcquired ())
    {
    case 1:
      acqusitionOk++;
      break;
    case -1:
      acqusitionFailed++;
      break;
    }
  ret = in_target->postprocess ();
  if (!ret)
    targetsQue.push_back (in_target);
  else
    delete in_target;
}

void
Rts2Executor::updateScriptCount ()
{
  scriptCount->setValueInteger (0);
  postEvent (new Rts2Event (EVENT_MOVE_QUESTION, (void *) scriptCount));
}

int
main (int argc, char **argv)
{
  Rts2Executor executor = Rts2Executor (argc, argv);
  return executor.run ();
}
