#pragma once

#include <string>
#include <memory>
#include <map>
#include <vector>

#include "../rapidxml/rapidxml.hpp"

#include "types.h"

class INotifyPropertyChanged;
struct BindingDef
{
    INotifyPropertyChanged* a;
    std::string a_prop;
    std::string b_name;
    std::string b_prop;
};
typedef std::vector<rapidxml::xml_attribute<>*> AttrBag;
typedef std::vector<BindingDef> BindingBag;
typedef std::map<INotifyPropertyChanged*, 
                 std::shared_ptr<INotifyPropertyChanged>> ElementsMap;

class Container;
class IVisualElement;
class TypeFactory;

class Serializer
{
public:
    Serializer(const char* filename, std::shared_ptr<TypeFactory> factory = nullptr);
    
    std::shared_ptr<INotifyPropertyChanged> deserialize();
private:
    void parse_container(Container* container, 
                         rapidxml::xml_node<>* node, 
                         const std::string& name,
                         const AttrBag& bag,
                         BindingBag& bindings,
                         ElementsMap& elements);

    std::shared_ptr<INotifyPropertyChanged> deserialize(IVisualElement* parent,
                                                        rapidxml::xml_node<>* node,
                                                        const AttrBag& bag,
                                                        BindingBag& bindings,
                                                        ElementsMap& elements);

    std::vector<char> _buffer;
    rapidxml::xml_document<> _doc;
    std::shared_ptr<TypeFactory> _factory;
};

