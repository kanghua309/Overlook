#include "Overlook.h"

namespace Overlook {

QtStats::QtStats() {
	corr_period = 4;
	max_timesteps = 3;
	steps = 8;
	peek = 0;
	
	arg_usetime			= 1;
	arg_usetrend		= 1;
	arg_usecorr			= 1;
}

void QtStats::Init() {
	peek = 2;
	
	
	System& bs = GetSystem();
	int sym_count = bs.GetBrokerSymbolCount();
	int tf_count = bs.GetPeriodCount();
	
	// Set how far into future bars are being drawn
	int tf = GetTimeframe();
	int max_tf = Upp::min(tf + 2, tf_count-1);
	if (max_tf > tf) {
		int future_bars = bs.GetPeriod(max_tf) / bs.GetPeriod(tf) * 2;
		SetFutureBars(future_bars);
	}
	
	
	SetCoreSeparateWindow();
	SetCoreMinimum(-1);
	SetCoreMaximum(+1);
	for(int i = 0; i < max_timesteps; i++) {
		SetBufferColor(i, RainbowColor(i / 3.0));
		SetBufferLineWidth(i, 3-i);
	}
	
	// Add targets
	// - types: change, trending, zigzag, edge, mid-channel
	qt.AddColumn("Target signal", steps);
	qt.EndTargets();
	
	// Add constant time values
	//  Note: adding month and day causes underfitting
	if (arg_usetime) {
		qt.AddColumn("Wday",			7);
		qt.AddColumn("Hour",			24);
		qt.AddColumn("5-min",			12);
	}
	
	// Add previous values
	if (arg_usetrend) {
		for (int sym = 0; sym < sym_count; sym++) {
			if (sym == GetSymbol()) continue; // skip this
			qt.AddColumn("Cur diff -1 (" + IntStr(sym) + ")", steps);
		}
	}
	
	// Add correlation values
	if (arg_usecorr) {
		for (int sym = 0; sym < sym_count; sym++) {
			if (sym == GetSymbol()) continue; // skip this
			qt.AddColumn("Correlation (" + IntStr(sym) + ")", steps);
		}
	}
	
}

void QtStats::Start() {
	int id = GetSymbol();
	int thistf = GetTf();
	int bars = GetBars();
	int counted = GetCounted();
	System& sys = GetSystem();
	int sym_count = sys.GetTotalSymbolCount();
	int tf_count = sys.GetPeriodCount();
	if (bars == counted)
		return;
	
	
	// Get some useful values for all syms and tfs
	Vector<ConstBuffer*> bufs;
	Vector<double> max_changes, min_changes, diffs;
	bufs.SetCount(sym_count, NULL);
	max_changes.SetCount(sym_count, 0);
	min_changes.SetCount(sym_count, 0);
	diffs.SetCount(sym_count, 0);
	
	for (int sym = 0; sym < sym_count; sym++) {
		bufs[sym] = &GetInputBuffer(0, sym, thistf, 0);
		DataBridge& db = *dynamic_cast<DataBridge*>(GetInputCore(0, sym, thistf));
		double max_change = db.GetMedianMax() * 2;
		double min_change = db.GetMedianMin() * 2;
		double diff = max_change - min_change;
		//LOG(Format("i=%d sym=%d tf=%d max=%f min=%f diff=%f", i, sym, tf, max_change, min_change, diff));
		max_changes[sym] = max_change;
		min_changes[sym] = min_change;
		diffs[sym] = diff;
	}
	
	double max_change = max_changes[id];
	double min_change = min_changes[id];
	double diff       = diffs[id];
	//LOG(Format("i=%d sym=%d tf=%d max=%f min=%f diff=%f", ii, id, 0, max_change, min_change, diff));
	
	peek = (int)pow(2, max_timesteps-1);
	bars -= peek;
	
	qt.Reserve(bars);
	
	TimeStop ts;
	VectorMap<int, int> in_qt;
	in_qt.Reserve(bars - counted);
	for (int i = counted; i < bars; i++) {
		SetSafetyLimit(i+peek);
		
		// Only add valid data to avoid underfitting
		double open = Open(i);
		double close = Open(i+1);
		if (open == close) continue;
		
		// Add row
		int row = qt.GetCount();
		in_qt.Add(i, row);
		qt.SetCount(row+1);
		
		// Get some time values in binary format (starts from 0)
		Time t = sys.GetTimeTf(thistf, i);
		int month = t.month-1;
		int day = t.day-1;
		int hour = t.hour;
		int minute = t.minute;
		int dow = DayOfWeek(t);
		int pos = 0;
		
		// Add target value after timestep
		int v = 0; // get ideal signal
		Panic("TODO");
		qt.Set(row, pos++, v);
		
		// Add constant time values
		if (arg_usetime) {
			qt.Set(row, pos++, dow);
			qt.Set(row, pos++, hour);
			qt.Set(row, pos++, minute / 5);
		}
		
		// Add previous changes in value
		if (arg_usetrend) {
			for (int sym = 0, csym = 0; sym < sym_count; sym++) {
				if (sym == GetSymbol()) continue; // skip this
				ConstBuffer& buf = *bufs[sym];
				double open = i > 0 ? buf.GetUnsafe(i-1) : 0.0;
				double cur = buf.GetUnsafe(i);
				double min_change = min_changes[sym];
				double diff = diffs[sym];
				double step = diff / steps;
				
				// Difference to previous time-position
				double change = open != 0.0 ? cur - open : 0.0;
				int v = (change - min_change) / step;
				if (v < 0) v = 0;
				if (v >= steps) v = steps -1;
				qt.Set(row, pos++, v);
				
				csym++;
			}
		}
		
		// Add correlation values
		if (arg_usecorr) {
			for (int sym = 0, csym = 0; sym < sym_count; sym++) {
				if (sym == GetSymbol()) continue; // skip this
				
				// Correlation to the main symbol
				ConstBuffer& cbuf = GetInputBuffer(2, GetSymbol(), thistf, csym);
				double corr = cbuf.GetUnsafe(i);
				v = (int)((corr + 1.0) / 2.0 * steps);
				if (v == steps) v = steps - 1; // equal to 1.0 doesn't need own range
				ASSERT(v >= 0 && v < steps);
				qt.Set(row, pos++, v);
				
				csym++;
			}
		}
		
		ASSERT(pos == qt.GetColumnCount());
	}
	
	// Create decision trees
	/*tree.Clear();
	tree.SetCount(max_timesteps);
	for(int i = 0; i < tree.GetCount(); i++)
		qt.GetDecisionTree(i, tree[i], qt.GetCount());
	
	// Draw forecast
	int peekbars = bars + peek;
	for (int i = counted; i < peekbars; i++) {
		
		// Skip invalid values, which weren't added to the query-table
		int j = in_qt.Find(i);
		if (j == -1) {
			for(int j = 0; j < max_timesteps; j++)
				GetBuffer(j).Set(i, -1);
			continue;
		}
		int row = in_qt[j];
		
		// Get prediction and correct value
		for(int j = 0; j < max_timesteps; j++) {
			double predicted = qt.Predict(tree[j], row, j);
			double correct = qt.Get(row, j);
			double diff = fabs(predicted - correct) / steps;
			GetBuffer(j).Set(i, diff);
		}
	}*/
	
	// Export target signal
	for (int i = counted; i < bars; i++) {
		
		// Skip invalid values, which weren't added to the query-table
		/*int j = in_qt.Find(i);
		if (j == -1) {
			for(int j = 0; j < max_timesteps; j++)
				GetBuffer(j).Set(i, -1);
			continue;
		}
		int row = in_qt[j];
		
		// Get prediction and correct value
		for(int j = 0; j < max_timesteps; j++) {
			double predicted = qt.Predict(tree[j], row, j);
			double correct = qt.Get(row, j);
			double diff = fabs(predicted - correct) / steps;
			GetBuffer(j).Set(i, diff);
		}*/
	}
}

}
