#include <memory>

#include "ui.h"
#include "parser.h"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

INITIALIZE_EASYLOGGINGPP

using namespace std;

void setup_ui(IVisualElement* c)
{
    auto btn_next = c->find_element("btnNext");
	auto btn_prev = c->find_element("btnPrev");
	auto page = dynamic_cast<Container*>(c->find_element("page"));
	shared_ptr<int> page_id(new int(0));
	
	for (auto p : page->get_elements())
	{
	    p->set_on_click([=](){
	        LOG(INFO) << "element " << p->get_name() << " was clicked!";
	        auto status = dynamic_cast<TextBlock*>(c->find_element("txtStatus"));
	        stringstream ss; ss << "element " << p->get_name() << " was clicked!";
	        status->set_text(ss.str());
	    });
	}
	
	c->find_element("btnChangeText")->set_on_click([=](){
	    auto status = dynamic_cast<TextBlock*>(c->find_element("txtTestWidth"));
	    status->set_text("very long long text!");
	});
	
	btn_next->set_on_click([page_id, page]() {
	    *page_id = (*page_id + 1) % page->get_elements().size();
	    LOG(INFO) << "moving to page " << *page_id << " out of " 
	              << page->get_elements().size();
	    stringstream ss; ss << *page_id;
	    auto child = page->find_element(ss.str());
	    page->set_focused_child(child);
	});
	btn_prev->set_on_click([page_id, page]() {
	    *page_id = (page->get_elements().size() + *page_id - 1) 
	                % page->get_elements().size();
	    LOG(INFO) << "moving to page " << *page_id << " out of " 
	              << page->get_elements().size();
	    stringstream ss; ss << *page_id;
	    auto child = page->find_element(ss.str());
	    page->set_focused_child(child);
	});
}

int main(int argc, char * argv[]) try
{
    glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 960, "main", 0, 0);
    glfwMakeContextCurrent(win);

    // create root-level container for the GUI
	Panel c(".",{0,0},{1.0f, 1.0f},0,Alignment::left,nullptr); 
	
	try
	{
	    LOG(INFO) << "Loading UI...";
	    Serializer s("ui.xml");
	    c.add_item(s.deserialize());
	    setup_ui(&c);
	} catch(const std::exception& ex) {
	    LOG(ERROR) << "UI Loading Error: " << ex.what();
	    stringstream ss; ss << "UI Loading Error: \"" << ex.what() << "\"";
	    c.add_item(std::shared_ptr<TextBlock>(new TextBlock(
	            "txtError",
                ss.str(),
                Alignment::left,
                {0,0},
                {0,0},
                0,
                { 1.0f, 0.2f, 0.2f }, 
                &c
	        )));
	}

    glfwSetWindowUserPointer(win, &c);
    glfwSetCursorPosCallback(win, [](GLFWwindow * w, double x, double y) {
        auto ui_element = (IVisualElement*)glfwGetWindowUserPointer(w);
        ui_element->update_mouse_position({ (int)x, (int)y });
    });
    glfwSetScrollCallback(win, [](GLFWwindow * w, double x, double y) {
        auto ui_element = (IVisualElement*)glfwGetWindowUserPointer(w);
        ui_element->update_mouse_scroll({ (int)x, (int)y });
    });
    glfwSetMouseButtonCallback(win, [](GLFWwindow * w, int button, int action, int mods)
    {
        auto ui_element = (IVisualElement*)glfwGetWindowUserPointer(w);
        MouseButton button_type;
        switch(button)
        {
        case GLFW_MOUSE_BUTTON_RIGHT:
            button_type = MouseButton::right; break;
        case GLFW_MOUSE_BUTTON_LEFT:
            button_type = MouseButton::left; break;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            button_type = MouseButton::middle; break;
        default:
            button_type = MouseButton::left;
        };

        MouseState mouse_state;
        switch(action)
        {
        case GLFW_PRESS:
            mouse_state = MouseState::down;
            break;
        case GLFW_RELEASE:
            mouse_state = MouseState::up;
            break;
        default:
            mouse_state = MouseState::up;
        };

        ui_element->update_mouse_state(button_type, mouse_state);
    });

    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        int w,h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        glPushMatrix();
        glfwGetWindowSize(win, &w, &h);
        glOrtho(0, w, h, 0, -1, +1);

        Rect origin { { 0, 0 }, { w, h } };

        c.render(origin);

        glPopMatrix();
        glfwSwapBuffers(win);
    }

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
catch(const std::exception & e)
{
    LOG(ERROR) << "Program crashed! " << e.what();
    return -1;
}
