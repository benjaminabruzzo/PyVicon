#include <Python.h>
#include <string>
#include "Client.h"

using namespace ViconDataStreamSDK;
using namespace CPP;

static PyObject* ViconError;

//--------------  Connect/disconnect/isConnected functions -------------

static PyObject* pyvicon_connect(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0); 
    char* input;
    
    // parse arguments
    if (!PyArg_ParseTuple(args, "s", &input))
        return NULL;
    
    // throw and error if already connected
    // although this probably comes out in the Result..
    if (client->IsConnected().Connected) {
        PyErr_SetString(ViconError, "connect() PyVicon already connected");
        return NULL;
    }
    
    // connect, ignore most results
    Output_Connect out = client->Connect(input);
    if (out.Result == Result::Success)
        return Py_True; // they can figure out the rest
    
    return Py_False;
}

static PyObject* pyvicon_disconnect(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    
    //raise errors
    if (client->IsConnected().Connected) {
        PyErr_SetString(ViconError, "disconnect() PyVicon already connected");
        return NULL;
    }
    
    //run disconnect, return none
    client->Disconnect();
    return Py_None;
}

static PyObject* pyvicon_isconnected(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    
    // status, as pybool
    return client->IsConnected().Connected ? Py_True : Py_False;
}

//------------------------- utility functions ------------------------

static PyObject* pyvicon_version(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    
    Output_GetVersion out = client->GetVersion();
    unsigned int version[3] = {out.Major, out.Minor, out.Point};
    
    return Py_BuildValue("(I,I,I)", version);
}

static PyObject* pyvicon_enablesegmentdata(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    return (client->EnableSegmentData().Result == Result::Success) ? Py_True : Py_False;
}

static PyObject* pyvicon_enablemarkerdata(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    return (client->EnableMarkerData().Result == Result::Success) ? Py_True : Py_False;
}

static PyObject* pyvicon_setstreammode(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    int input;
    StreamMode::Enum mode;
    
    //parse the argument
    if (!PyArg_ParseTuple(args, "i", &input))
        return NULL;
    if (input < 0 && input > 2) {
        PyErr_SetString(PyExc_TypeError, "setStreamMode() takes ints (0, 1, 2)");
        return NULL;
    }
    
    //convert to the enum
    switch (input) {
        case 0:
            mode = StreamMode::ClientPull;
            break;
        case 1:
            mode = StreamMode::ClientPullPreFetch;
            break;
        default:
            mode = StreamMode::ServerPush;
    }
    
    //return result
    return (client->SetStreamMode(mode).Result == Result::Success) ? Py_True : Py_False;
}

//------------------------ subject getters ----------------------------

static PyObject* pyvicon_subjectcount(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    return Py_BuildValue("I", client->GetSubjectCount().SubjectCount);
}

static PyObject* pyvicon_subjectname(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    unsigned int index;
    
    if (!PyArg_ParseTuple(args, "I", &index))
        return NULL;
    
    Output_GetSubjectName out = client->GetSubjectName(index);
    
    if (out.Result == Result::InvalidIndex) {
        PyErr_SetString(ViconError, "subjectName() Invalid Index");
        return NULL;
    }
    
    //cast from the silly vicon string type
    std::string sub_name = (std::string)out.SubjectName;
    return Py_BuildValue("s", sub_name.c_str());
}

static PyObject* pyvicon_subjects(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    
    //get number of subjects
    const unsigned int sub_count = client->GetSubjectCount().SubjectCount;
    PyObject* subjects = PyList_New(0);
    
    //collect subject names into the list (it's expandable!)
    for (unsigned int i=0; i<sub_count; i++) {
        std::string sub = (std::string)client->GetSubjectName(i).SubjectName;
        PyList_Append(subjects, Py_BuildValue("s", sub.c_str()));
    }
    return subjects;
}

//--------------------- rotation/translation getters ----------------------

static PyObject* pyvicon_globalrotation(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    char* name;
    
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    
    //check for segment data
    if (client->IsSegmentDataEnabled().Enabled) {
        PyErr_SetString(ViconError, "globalRotation() SegmentData not enabled");
        return NULL;
    }
    
    Output_GetSegmentGlobalRotationHelical out = client->GetSegmentGlobalRotationHelical(name, name);
    switch (out.Result) {
        case Result::Success:
            break; //keep going!!
        case Result::InvalidSegmentName:
            PyErr_SetString(ViconError, "globalRotation() Invalid Segment Name");
            return NULL;
        case Result::InvalidSubjectName:
            PyErr_SetString(ViconError, "globalRotation() Invalid Subject Name");
            return NULL;
        default:
            PyErr_SetString(ViconError, "globalRotation() Unknown Error - sorry!");
            return NULL;
    }
    
    //let python do the rest, just know it's a set of 3 doubles
    return Py_BuildValue("(d,d,d)", out.Rotation);
}

static PyObject* pyvicon_globaltranslation(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    char* name;
    
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    
    //check for segment data
    if (client->IsSegmentDataEnabled().Enabled) {
        PyErr_SetString(ViconError, "globalTranslation() SegmentData not enabled");
        return NULL;
    }
    
    Output_GetSegmentGlobalTranslation out = client->GetSegmentGlobalTranslation(name, name);
    switch (out.Result) {
        case Result::Success:
            break; //keep going!!
        case Result::InvalidSegmentName:
            PyErr_SetString(ViconError, "globalTranslation() Invalid Segment Name");
            return NULL;
        case Result::InvalidSubjectName:
            PyErr_SetString(ViconError, "globalTranslation() Invalid Subject Name");
            return NULL;
        default:
            PyErr_SetString(ViconError, "globalTranslation() Unknown Error - sorry!");
            return NULL;
    }
    
    return Py_BuildValue("(d,d,d)", out.Translation);
}

//------------------------ marker, frame, other --------------------------

static PyObject* pyvicon_frame(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    return (client->GetFrame().Result == Result::Success) ? Py_True : Py_False;
}

static PyObject* pyvicon_framenumber(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    return Py_BuildValue("I", client->GetFrameNumber().FrameNumber);
}

static PyObject* pyvicon_markercount(PyObject* self, PyObject* args) {
    Client* client = (Client*)PyCapsule_Import("pyvicon.client", 0);
    char* name;
    
    if (!PyArg_ParseTuple(args, "s", &name))
        return NULL;
    
    //check for marker data
    if (client->IsMarkerDataEnabled().Enabled) {
        PyErr_SetString(ViconError, "markerCount() MarkerData not enabled");
        return NULL;
    }
    
    return Py_BuildValue("I", client->GetMarkerCount(name).MarkerCount);
}

//------------------------- aaaaand the rest ----------------------------

//declare the accessible functions
static PyMethodDef ModuleMethods[] = {
     {"connect", pyvicon_connect, METH_VARARGS, "Connect to vicon"},
     {"disconnect", pyvicon_disconnect, METH_NOARGS, "Disconnect from vicon"},
     {"isConnected", pyvicon_isconnected, METH_NOARGS, "Connection status"},
     {"subjectCount", pyvicon_subjectcount, METH_NOARGS, "Get a count of subjects"},
     {"subjectName", pyvicon_subjectname, METH_VARARGS, "Get a subject name, given an index"},
     {"subjects", pyvicon_subjects, METH_NOARGS, "Get a list of all subjects"},
     {"globalRotation", pyvicon_globalrotation, METH_VARARGS, "Get global rotation of a subject"},
     {"globalTranslation", pyvicon_globaltranslation, METH_VARARGS, "Get global translation of a subject"},
     {"markerCount", pyvicon_markercount, METH_VARARGS, "Get number of visible markers of a subject"},
     {"frame", pyvicon_frame, METH_NOARGS, "I don't know, a status thing?"},
     {"frameNumber", pyvicon_framenumber, METH_NOARGS, "Current frame number"},
     {"setStreamMode", pyvicon_setstreammode, METH_VARARGS, "Stream mode: Pull, PreFetch, Push"},
     {"enableSegmentData", pyvicon_enablesegmentdata, METH_NOARGS, "Enables segment data. Just always use it, I guess."},
     {"enableMarkerData", pyvicon_enablemarkerdata, METH_NOARGS, "Enables marker data. Use this for markerCount."},
     {"version", pyvicon_version, METH_NOARGS, "Vicon system version"},
     {NULL, NULL, 0, NULL},
};

PyMODINIT_FUNC initpyvicon(void) {
    //create the module
    PyObject* m;
    m = Py_InitModule("pyvicon", ModuleMethods);
    
    //create vicon client and add to the module - inside a capsule
    Client* vicon_client = new Client();
    PyObject* pyclient = PyCapsule_New(vicon_client, "pyvicon.client", NULL);
    PyModule_AddObject(m, "client", pyclient);
    
    //create+add ViconError
    ViconError = PyErr_NewException("pyvicon.error", NULL, NULL);
    Py_INCREF(ViconError);
    PyModule_AddObject(m, "error", ViconError);
    
    //add StreamMode enum, not that we use anything but clientpull
    PyObject* sm1 = Py_BuildValue("i", 0);
    PyObject* sm2 = Py_BuildValue("i", 1);
    PyObject* sm3 = Py_BuildValue("i", 2);
    
    PyObject_SetAttrString(m, "SM_ClientPull", sm1);
    PyObject_SetAttrString(m, "SM_ClientPullPreFetch", sm2);
    PyObject_SetAttrString(m, "SM_ServerPush", sm3);
    
    //clean up
    Py_DECREF(sm1);
    Py_DECREF(sm2);
    Py_DECREF(sm3);
}
