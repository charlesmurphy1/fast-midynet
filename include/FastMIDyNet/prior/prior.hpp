#ifndef FAST_MIDYNET_PRIOR_HPP
#define FAST_MIDYNET_PRIOR_HPP


#include <functional>
#include "FastMIDyNet/types.h"
#include "FastMIDyNet/proposer/movetypes.h"


namespace FastMIDyNet{

template <typename StateType>
class Prior{
    public:
        Prior<StateType>(){}
        Prior<StateType>(const Prior<StateType>& other):
            m_state(other.m_state){}
        virtual ~Prior<StateType>(){}
        const Prior<StateType>& operator=(const Prior<StateType>& other){
            this->m_state = other.m_state;
            return *this;
        }

        const StateType& getState() const { return m_state; }
        StateType& getStateRef() const { return m_state; }
        virtual void setState(const StateType& state) { m_state = state; }

        virtual void sampleState() = 0;
        virtual void samplePriors() = 0;
        void sample() {
            processRecursiveFunction([&]() {
                    samplePriors();
                    sampleState();
                });
        }
        virtual double getLogLikelihood() const = 0;
        virtual double getLogPrior() = 0;

        double getLogJoint() {
            return processRecursiveFunction<double>( [&]() { return getLogPrior()+getLogLikelihood(); }, 0);
        }

        virtual void checkSelfConsistency() const = 0;
        virtual void computationFinished() { m_isProcessed = false; }


        template<typename RETURN_TYPE>
        RETURN_TYPE processRecursiveFunction(const std::function<RETURN_TYPE()>& func, RETURN_TYPE init) {
            RETURN_TYPE ret = init;
            if (!m_isProcessed)
                ret = func();
            m_isProcessed=true;
            return ret;
        }
        void processRecursiveFunction(const std::function<void()>& func) {
            if (!m_isProcessed)
                func();
            m_isProcessed=true;
        }

    protected:
        StateType m_state;
        bool m_isProcessed = false;
};

}

#endif
