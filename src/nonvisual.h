#pragma once
#include "ui.h"

class Timer : public BindableObjectBase
{
public:
    Timer()
    {
        _started = std::chrono::high_resolution_clock::now();
    }
    
    int get_elapsed() const
    {
        auto elapsed = (std::chrono::high_resolution_clock::now() - _started);
        return std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
    }
    
    void update() override
    {
        auto elapsed = get_elapsed();
        if (elapsed != _elapsed)
        {
            _elapsed = elapsed;
            fire_property_change("elapsed");
        }
    }
    
private:
    std::chrono::high_resolution_clock::time_point _started;
    int _elapsed = 0;
};

template<>
struct TypeDefinition<Timer>
{
    static std::shared_ptr<ITypeDefinition> make() 
    {
        DefineClass(Timer)->AddField(get_elapsed);
    }
};
