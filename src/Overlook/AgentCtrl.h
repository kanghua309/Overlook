#ifndef _Overlook_AgentCtrl_h_
#define _Overlook_AgentCtrl_h_

namespace Overlook {

class AgentDraw : public Ctrl {
	Agent* agent;
	int snap_id;
	
public:
	AgentDraw();
	
	virtual void Paint(Draw& w);
	void SetSnap(int i) {snap_id = i;}
	
};

class StatsGraph : public Ctrl {
	
protected:
	Agent* agent;
	Vector<Point> polyline;
	Vector<double> last;
	
public:
	typedef StatsGraph CLASSNAME;
	StatsGraph();
	
	virtual void Paint(Draw& w);
};

class AgentThreadCtrl : public ParentCtrl {
	
protected:
	friend class AgentDraw;
	
	Agent* agent;
	SliderCtrl time_slider;
	Splitter hsplit;
	AgentDraw draw;
	BrokerCtrl brokerctrl;
	int thrd_id;
	
public:
	typedef AgentThreadCtrl CLASSNAME;
	AgentThreadCtrl();
	
	void Data();
	
};

class AgentCtrl : public ParentCtrl {
protected:
	Agent* agent;
	Array<AgentThreadCtrl> thrds;
	StatsGraph reward;
	DropList thrdlist;
	Option update_brokerctrl;
	bool init;
	
public:
	typedef AgentCtrl CLASSNAME;
	AgentCtrl();
	
	void Data();
	void SetView();
	
};

class TrainingGraph : public Ctrl {
	Agent* agent;
	Vector<Point> polyline;
	
public:
	typedef TrainingGraph CLASSNAME;
	TrainingGraph();
	
	virtual void Paint(Draw& w);
};

class AgentTraining : public ParentCtrl {
	
protected:
	friend class AgentDraw;
	Agent* agent;
	Splitter hsplit;
	bool init;
	
	Option paused, prefer_highresults;
	Label epoch;
	AgentDraw draw;
	TrainingGraph reward;
	ConvNet::SessionConvLayers conv;
	ConvNet::HeatmapTimeView timescroll;
	
	Splitter leftctrl;
	ArrayCtrl seslist;
	ParentCtrl settings;
	Label lrate, lmom, lbatch, ldecay;
	EditDouble rate, mom, decay;
	EditInt batch;
	Button apply;
	ConvNet::TrainingGraph graph;
	
public:
	typedef AgentTraining CLASSNAME;
	AgentTraining();
	
	void Data();
	void ApplySettings();
	void SetPreferHigh() {agent->prefer_high = prefer_highresults.Get();}
	void SetPaused() {agent->paused = paused.GetData();}
	void SetAgent(Agent& agent) {this->agent = &agent;}
	
};


class RealtimeNetworkCtrl : public ParentCtrl {
	Agent* agent;
	RealtimeSession* rtses;
	
	Button refresh_signals, killall_signals;
	Splitter hsplit;
	AgentDraw draw;
	BrokerCtrl brokerctrl;
	Label tfcmplbl;
	
public:
	typedef RealtimeNetworkCtrl CLASSNAME;
	RealtimeNetworkCtrl();
	
	void Data();
	void RefreshSignals();
	void KillSignals();
	
};


class SnapshotCtrl : public ParentCtrl {
	AgentGroup* group;
	Splitter hsplit;
	ArrayCtrl list;
	AgentDraw draw;
	
public:
	typedef SnapshotCtrl CLASSNAME;
	SnapshotCtrl();
	
	void SetGroup(AgentGroup& group) {this->group = &group;}
	void Data();
};

}

#endif
