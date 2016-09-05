#include "containers.h"

#include "../easyloggingpp/easylogging++.h"

using namespace std;

void StackPanel::update_mouse_position(Int2 cursor)
{
    Size Size2::* field;
    int Int2::* ifield;
    if (_orientation == Orientation::vertical) {
        field = &Size2::y;
        ifield = &Int2::y;
    } else {
        field = &Size2::x;
        ifield = &Int2::x;
    }

    auto sum = get_arrangement().position.*ifield;
    set_focused_child(nullptr);
    for (auto& p : get_elements()) {
        auto new_origin = get_arrangement();
        (new_origin.position.*ifield) = sum;
        (new_origin.size.*ifield) = _sizes[p.get()].first;
        sum += _sizes[p.get()].first;

        //cout << new_origin << " - " << cursor << endl;

        if (contains(new_origin, cursor))
        {
            set_focused_child(p.get());
            p->focus(true);
            p->update_mouse_position(cursor);
        }
        else
        {
            p->focus(false);
        }
    }
}

void Container::add_item(shared_ptr<IVisualElement> item)
{
    invalidate_layout();

    auto panel = dynamic_cast<Container*>(item.get());
    if (panel)
    {
        panel->set_items_change([this](){
            invalidate_layout();
        });
    }

    _on_items_change();
    
    _focused = item.get();

    _content.push_back(item);
}

SizeMap StackPanel::calc_sizes(Orientation orientation,
                              const Elements& content,
                              const Rect& arrangement)
{
    SizeMap sizes;

    auto sum = 0;

    Size Size2::* field;
    int Int2::* ifield;
    Size Size2::* other;
    if (orientation == Orientation::vertical) {
        field = &Size2::y;
        ifield = &Int2::y;
        other = &Size2::x;
    } else {
        field = &Size2::x;
        ifield = &Int2::x;
        other = &Size2::y;
    }

    // first, scan items, map the "greedy" ones wanting relative portion
    vector<IVisualElement*> greedy;
    for (auto& p : content) {
        auto p_rect = p->arrange(arrangement);
        auto p_total = p->get_margin().apply(p_rect);
        auto p_size = p->get_size();
        LOG(INFO) << "child " << p->get_name() << " returned size " << p_size;

        if ((p_size.*field).is_const()) {
            auto pixels = (p_size.*field).get_pixels();
            sum += pixels;
            sizes[p.get()].first = pixels;
            sizes[p.get()].second = p_size.*other;
        } else {
            greedy.push_back(p.get());
        }
    }

    auto rest = max(arrangement.size.*ifield - sum, 0);
    float total_parts = 0.0001;
    for (auto ptr : greedy) {
        total_parts += (ptr->get_size().*field).get_percents();
    }
    for (auto ptr : greedy) {
        auto f = ((ptr->get_size().*field).get_percents() / total_parts);
        sizes[ptr].first = (int) (rest * f);
        sizes[ptr].second = ptr->get_size().*other;
    }

    return sizes;
}

Size2 StackPanel::get_intrinsic_size() const
{
    auto sizes = calc_global_sizes({ { 0, 0 }, { 0, 0 } });
    auto total = 0;
    auto max = 0;
    for (auto& kvp : sizes) {
        auto pixels = kvp.second.first;
        auto size = kvp.second.second;

        total += pixels;
        max = std::max(max, size.to_pixels(0));
    }

    if (_orientation == Orientation::vertical) {
        return { Size(max), Size(total) };
    } else {
        return { Size(total), Size(max) };
    }
}

void StackPanel::render(const Rect& origin)
{
    Size Size2::* field;
    int Int2::* ifield;
    if (_orientation == Orientation::vertical) {
        field = &Size2::y;
        ifield = &Int2::y;
    } else {
        field = &Size2::x;
        ifield = &Int2::x;
    }

    if (update_layout(origin))
    {
        _sizes = calc_global_sizes(get_arrangement());
        
        LOG(INFO) << "New layout for element " << get_name() << ":";
        for (auto& kvp : _sizes)
        {
            LOG(INFO) << "\t" << kvp.first->get_name() << " = " << kvp.second.first;
        }
    }

    auto sum = get_arrangement().position.*ifield;
    for (auto& p : get_elements()) {
        auto new_origin = get_arrangement();
        (new_origin.position.*ifield) = sum;
        (new_origin.size.*ifield) = _sizes[p.get()].first;
        sum += _sizes[p.get()].first;
        p->render(new_origin);
    }
}

SimpleSizeMap Panel::calc_size_map(const Elements& content,
                                   const Rect& arrangement)
{
    SimpleSizeMap map;
    for (auto& p : content)
    {
        map[p.get()] = p->arrange(arrangement).size;
    }
    return map;
}

void Panel::render(const Rect& origin)
{
    for (auto& p : get_elements()) {
        p->render(origin);
    }
}

Size2 Panel::get_intrinsic_size() const
{
    auto sizes = calc_size_map(get_elements(), { { 0, 0 }, { 0, 0 } });
    auto max_x = 0;
    auto max_y = 0;
    for (auto& kvp : sizes) {
        auto x = kvp.second.x;
        auto y = kvp.second.y;

        max_x = std::max(max_x, x);
        max_y = std::max(max_y, y);
    }

    return { Size(max_x), Size(max_y) };
}

void PageView::render(const Rect& origin)
{
    get_focused_child()->render(origin);
}

Size2 PageView::get_intrinsic_size() const
{
    if (get_focused_child())
        return get_focused_child()->get_intrinsic_size();
}

void Grid::commit_line()
{
    if (_current_line) {
        StackPanel::add_item(_current_line);
        _lines.push_back(_current_line);
    }
    _current_line = shared_ptr<StackPanel>(
        new StackPanel("", {0,0}, { 1.0f, 1.0f }, 0, get_alignment(),
        get_orientation() == Orientation::vertical 
            ? Orientation::horizontal : Orientation::vertical,
        this));
    _current_line->update_parent(this);
}

SizeMap Grid::calc_sizes(const StackPanel* sender,
                         const Rect& arrangement) const
{
    map<const StackPanel*, SizeMap> maps;
    auto max_length = 0;
    for (auto& p : _lines)
    {
        auto map = p->calc_local_sizes(arrangement);
        maps[p.get()] = map;
        max_length = max(max_length, (int)map.size());
    }
    
    vector<vector<int>> grid;
    for (auto& p : _lines)
    {
        vector<int> row(max_length, 0);
        auto i = 0;
        for (auto& kvp : maps[p.get()])
        {
            row[i++] = kvp.second.first;
        }
        grid.push_back(row);
    }
    
    auto result = maps[sender];
    
    auto _index = 0;
    for (auto& kvp : result)
    {
        auto max = 0;
        for (auto j = 0; j < grid.size(); j++)
        {
            max = std::max(grid[j][_index], max);
        }
        kvp.second.first = max;
        _index++;
    }
    
    return result;
}

