#ifndef	_NBL_SYSTEM_I_APPLICATION_FRAMEWORK_H_INCLUDED_
#define	_NBL_SYSTEM_I_APPLICATION_FRAMEWORK_H_INCLUDED_

namespace nbl::system
{
	class IApplicationFramework : public core::IReferenceCounted
	{
	public:
        IApplicationFramework(const system::path& _cwd) : CWD(_cwd)
		{

		}
        void onAppInitialized(void* data)
        {
            return onAppInitialized_impl(data);
        }
        void onAppTerminated(void* data)
        {
            return onAppTerminated_impl(data);
        }
        virtual void workLoopBody(void* params) = 0;
        virtual bool keepRunning(void* params) = 0;
    protected:
        virtual void onAppInitialized_impl(void* data) {}
        virtual void onAppTerminated_impl(void* data) {}
    protected:
        system::path CWD;
    };
}

#endif