
#ifndef Receiver_SS_class_H
#define Receiver_SS_class_H

#include "ss_api.hxx"

class ReceiverX;

namespace Receiver_SS
{
	using namespace smartstate;
	//State Mgr
	class ReceiverSS : public StateMgr
	{
		public:
			ReceiverSS(ReceiverX* ctx, bool startMachine=true);

			ReceiverX& getCtx() const;

			bool isRunning() const;
			void postEvent(unsigned int event, int wParam = 0,
				int lParam = 0) throw (std::string);

		protected:
			int state; // for HandCoded version

		private:
			ReceiverX* myCtx;
	};
};

#endif

