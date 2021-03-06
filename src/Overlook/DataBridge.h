#ifndef _Overlook_DataBridge_h_
#define _Overlook_DataBridge_h_

namespace Config {
extern IniBool use_internet_m1_data;
}

namespace Overlook {


class DataBridge;

class DataBridgeCommon {
	
protected:
	friend class DataBridge;
	
	typedef Tuple3<Time, double, double> AskBid;
	Vector<Vector<AskBid> > data;
	Vector<int> tfs;
	Vector<bool> loaded;
	Vector<double> points;
	Index<String> short_ids;
	String account_server;
	String addr;
	TimeStop since_last_askbid_refresh;
	int port;
	int sym_count;
	int cursor;
	bool connected;
	bool inited;
	
	
	void Init(DataBridge* db);
public:
	DataBridgeCommon();
	void CheckInit(DataBridge* db);
	
	const Symbol& GetSymbol(int i) const {return GetMetaTrader().GetSymbol(i);}
	int GetSymbolCount() const {return GetMetaTrader().GetSymbolCount();}
	int GetTf(int i) const {return tfs[i];}
	int GetTfCount() const {return tfs.GetCount();}
	void DownloadRemoteData();
	int  DownloadHistory(const Symbol& sym, int tf, bool force=false);
	int  DownloadAskBid();
	int  DownloadRemoteFile(String remote_path, String local_path);
	bool IsInited() const {return inited;}
	void RefreshAskBidData(bool forced=false);
	
	Mutex lock;
};

inline DataBridgeCommon& GetDataBridgeCommon() {return Single<DataBridgeCommon>();}

struct AskBid : Moveable<AskBid> {
	int time;
	double ask, bid;
};

class DataBridge : public BarData {
	VectorMap<int,int> median_max_map, median_min_map;
	VectorMap<int,int> symbols;
	Vector<Vector<byte> > ext_data;
	Vector<Vector<int> > sym_group_stats, sym_groups;
	double spread_mean;
	int spread_count;
	int median_max, median_min;
	int max_value, min_value;
	int cursor, buffer_cursor;
	int data_begin;
	bool slow_volume, day_volume;
	
	void RefreshFromHistory(bool use_internet_data, bool update_only=false);
	void RefreshFromInternet();
	void RefreshFromAskBid(bool init_round);
	void RefreshVirtualNode();
	void RefreshCorrelation();
	void RefreshBasket();
	void RefreshMedian();
public:
	typedef DataBridge CLASSNAME;
	DataBridge();
	~DataBridge();
	
	virtual void IO(ValueRegister& reg) {
		reg % In<DataBridge>(&FilterFunction)
			% Out(4, 3)
			% Persistent(spread_mean) % Persistent(spread_count)
			% Persistent(cursor) % Persistent(buffer_cursor)
			% Persistent(data_begin)
			% Persistent(median_max_map) % Persistent(median_min_map)
			% Persistent(symbols)
			% Persistent(ext_data)
			% Persistent(sym_group_stats) % Persistent(sym_groups)
			% Persistent(median_max) % Persistent(median_min)
			% Persistent(max_value) % Persistent(min_value);
	}
	
	virtual void Init();
	virtual void Start();
	
	int GetChangeStep(int shift, int steps);
	int GetDataBegin() const {return data_begin;}
	double GetMax() const {return max_value * point;}
	double GetMin() const {return min_value * point;}
	double GetMedianMax() const {return median_max * point;}
	double GetMedianMin() const {return median_min * point;}
	double GetAverageSpread() const {return spread_mean;}
	void AddSpread(double a);
	
	static bool FilterFunction(void* basesystem, int in_sym, int in_tf, int out_sym, int out_tf) {
		// Add this timeframe
		if (in_sym == -1)
			return in_tf == out_tf;
		
		// Never for regular symbols
		MetaTrader& mt = GetMetaTrader();
		int sym_count = mt.GetSymbolCount();
		int cur_count = mt.GetCurrencyCount();
		int corr_sym = sym_count + cur_count;
		if (in_sym < sym_count)
			return false;
		
		// Never for irregular input symbols
		if (out_sym >= sym_count && out_sym != corr_sym)
			return false;
		
		// All regular symbols for correlation and baskets
		if (in_sym == corr_sym)
			return out_sym < sym_count;
		if (in_sym > corr_sym)
			return out_sym < sym_count || out_sym == corr_sym;
		
		// Get virtual symbol
		int cur = in_sym - sym_count;
			const Currency& c = mt.GetCurrency(cur);
			if (c.pairs0.Find(out_sym) != -1 || c.pairs1.Find(out_sym) != -1)
				return true; // add pair sources add inputs
		
		return false;
	}
};



class ValueChange : public Core {
	
public:
	ValueChange();
	
	virtual void IO(ValueRegister& reg) {
		reg % In<DataBridge>(&FilterFunction)
			% Out(3, 3);
	}
	
	virtual void Init();
	virtual void Start();
	
	static bool FilterFunction(void* basesystem, int in_sym, int in_tf, int out_sym, int out_tf) {
		if (in_sym == -1)	return in_tf  == out_tf;
		else				return in_sym == out_sym;
	}
};

}

#endif
