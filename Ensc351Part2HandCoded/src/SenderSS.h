#ifndef XMODEMSENDERSS_H_
#define XMODEMSENDERSS_H_

#include "ss_api.hxx"

class SenderX;

namespace Sender_SS
{
	using namespace smartstate;
	//State Mgr
	class SenderSS : public StateMgr
	{
		public:
			SenderSS(SenderX* ctx, bool startMachine=true);

			SenderX& getCtx() const;

			bool isRunning() const;
			void postEvent(unsigned int event, int wParam = 0,
				int lParam = 0) throw (std::string);

		protected:
			int state; // for HandCoded version

		private:
			SenderX* myCtx;
	};
}

#endif /*XMODEMSENDERSS_H_*/
