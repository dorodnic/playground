#include <memory>
#include <chrono>
#include <cmath>

#include "ui.h"
#include "controls.h"
#include "containers.h"
#include "serializer.h"
#include "font.h"

#ifdef WIN32
	#define USEGLEW
	#include <GL/glew.h>
#endif

#define GLFW_INCLUDE_GLU
#define GLFW_INCLUDE_GLEXT
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

INITIALIZE_EASYLOGGINGPP

using namespace std;

struct FloatCounter : public BindableObjectBase
{
    FloatCounter(int sign) 
        : _sign(sign), 
          name((sign > 0) ? "positive float" : "negative float") {}
    
    float count = 0.0f;

    void update()
    {
        count += _sign * 0.22f;
        fire_property_change("count");
    }
    
    std::shared_ptr<ITypeDefinition> make_type_definition() const override
    {
        DefineClass(FloatCounter)->AddField(count)->AddField(name);
    }
    
    int _sign;
    std::string name;
};

struct IntCounter : public BindableObjectBase
{
    IntCounter(int sign) 
        : _sign(sign), 
          name((sign > 0) ? "positive int" : "negative int") {}
    
    int count = 0;

    void update()
    {
        count += _sign;
        fire_property_change("count");
    }
    
    std::shared_ptr<ITypeDefinition> make_type_definition() const override
    {
        DefineClass(IntCounter)->AddField(count)->AddField(name);
    }
    
    int _sign;
    std::string name;
};

struct Context : public BindableObjectBase
{
    Context(int sign) 
        : floats_counter(new FloatCounter(sign)),
          ints_counter(new IntCounter(sign)),
          counter(nullptr),
          name((sign > 0) ? "positive DC" : "negative DC") {}

    std::shared_ptr<INotifyPropertyChanged> counter;
    
    std::shared_ptr<FloatCounter> floats_counter;
    std::shared_ptr<IntCounter> ints_counter;
    
    void set_int_mode()
    {
        counter = ints_counter;
        fire_property_change("counter");
    }
    
    void set_float_mode()
    {
        counter = floats_counter;
        fire_property_change("counter");
    }
    
    void set_null()
    {
        counter = nullptr;
        fire_property_change("counter");
    }
    
    void update_fps()
    {
        auto now = std::chrono::high_resolution_clock::now();

        _frame_times.erase(std::remove_if(_frame_times.begin(), _frame_times.end(), 
        [now](auto tp){
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - tp);
            return ms.count() >= 1000;
        }), _frame_times.end());
        
        _frame_times.push_back(now);
        
        if (_frame_times.size() != fps)
        {
            fps = _frame_times.size();
            fire_property_change("fps");
        }
    }
    
    void update()
    {
        floats_counter->update();
        ints_counter->update();
        
        update_fps();
    }
    
    std::shared_ptr<ITypeDefinition> make_type_definition() const override
    {
        DefineClass(Context)
            ->AddField(counter)
            ->AddField(name)
            ->AddField(fps);
    }
    
    std::string name;
    
    std::vector<std::chrono::high_resolution_clock::time_point> _frame_times;
    
    int fps = 0;
};

void setup_ui(IVisualElement* c, shared_ptr<INotifyPropertyChanged> dcPlus,
                                 shared_ptr<INotifyPropertyChanged> dcMinus)
{
    auto btn_next = c->find_element("btnNext");
	auto btn_prev = c->find_element("btnPrev");
	auto page = dynamic_cast<PageView*>(c->find_element("page"));
	shared_ptr<int> page_id(new int(atoi(page->get_focused_child()->get_name().c_str())));
	
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
	    status->set_text(status->get_text() + ", very long long text!");
	    LOG(INFO) << "adding more text!";
	});
	
	c->find_element("btnToggleEnabled")->set_on_click([=](){
	    auto btn = dynamic_cast<Button*>(c->find_element("btnTestButton"));
	    
	    auto self = dynamic_cast<Button*>(c->find_element("btnToggleEnabled"));
	    if (btn->is_enabled()) self->set_text("enable");
	    else self->set_text("disable");
	    
	    btn->set_enabled(!btn->is_enabled());
	    LOG(INFO) << "toggling is_enabled on and off";
	});
	
	c->find_element("btnToggleVisible")->set_on_click([=](){
	    auto btn = dynamic_cast<Button*>(c->find_element("btnTestButton"));
	    
	    auto self = dynamic_cast<Button*>(c->find_element("btnToggleVisible"));
	    if (btn->is_visible()) self->set_text("show");
	    else self->set_text("hide");
	    
	    btn->set_visible(!btn->is_visible());
	    LOG(INFO) << "toggling is_visible on and off";
	});
	
	c->find_element("btnTestButton")->set_on_click([=](){
	    auto btn = dynamic_cast<Button*>(c->find_element("btnTestButton"));
	    btn->set_color(-btn->get_color());
	    LOG(INFO) << "button was clicked!";
	});
	
	auto btn_change_dc_to_null = c->find_element("btnChangeDCtoNull");	
	auto btn_change_dc_to_plus = c->find_element("btnChangeDCtoPlus");
	auto btn_change_dc_to_minus = c->find_element("btnChangeDCtoMinus");
	
	auto btn_change_counter_to_floats = c->find_element("btnChangeDCtoFloats");
	auto btn_change_counter_to_ints = c->find_element("btnChangeDCtoInts");
	auto btn_change_counter_to_null = c->find_element("btnChangeCounterToNull");
	
	auto grid = c->find_element("grid_with_dc");
	btn_change_dc_to_plus->set_on_click([dcPlus, grid]() {
	    LOG(INFO) << "Setting DC to plus";
	    grid->set_data_context(dcPlus);
	});
	btn_change_dc_to_minus->set_on_click([dcMinus, grid]() {
	    LOG(INFO) << "Setting DC to minus";
	    grid->set_data_context(dcMinus);
	});
	btn_change_dc_to_null->set_on_click([grid]() {
	    LOG(INFO) << "Setting DC to null";
	    grid->set_data_context(nullptr);
	});
	
	btn_change_counter_to_floats->set_on_click([grid]() {
	    LOG(INFO) << "Setting DC.counter to float";
	    auto dc = dynamic_cast<Context*>(grid->get_data_context().get());
	    if (dc) dc->set_float_mode();
	});
	btn_change_counter_to_ints->set_on_click([grid]() {
	    LOG(INFO) << "Setting DC.counter to int";
	    auto dc = dynamic_cast<Context*>(grid->get_data_context().get());
	    if (dc) dc->set_int_mode();
	});
	btn_change_counter_to_null->set_on_click([grid]() {
	    LOG(INFO) << "Setting DC.counter to null";
	    auto dc = dynamic_cast<Context*>(grid->get_data_context().get());
	    if (dc) dc->set_null();
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

GLuint load_shaders(const char* vertex_shader_file, 
                    const char* fragment_shader_file)
{
	// Create the shaders
	GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);


	auto vertex_shader_code = read_all_text(vertex_shader_file);
    auto fragment_shader_code = read_all_text(fragment_shader_file);

    LOG(INFO) << "Compiling shader " << vertex_shader_file << "...";
    
	char const * vertex_source_ptr = vertex_shader_code.c_str();
	int length = vertex_shader_code.size();
	glShaderSource(vertex_shader_id, 1, &vertex_source_ptr, &length);
	glCompileShader(vertex_shader_id);

	// Check Vertex Shader
	GLint result;
	int log_length;
	
	glGetShaderiv(vertex_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertex_shader_id, GL_INFO_LOG_LENGTH, &log_length);
	if ((result == GL_FALSE) && (log_length > 0)){
		std::vector<char> error_message(log_length+1);
		glGetShaderInfoLog(vertex_shader_id, log_length, NULL, &error_message[0]);
		LOG(ERROR) << &error_message[0];
		return 0;
	}

    LOG(INFO) << "Compiling shader " << fragment_shader_file << "...";
    
	char const * fragment_source_ptr = fragment_shader_code.c_str();
	glShaderSource(fragment_shader_id, 1, &fragment_source_ptr, NULL);
	glCompileShader(fragment_shader_id);

    glGetShaderiv(fragment_shader_id, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragment_shader_id, GL_INFO_LOG_LENGTH, &log_length);
	if ((result == GL_FALSE) && (log_length > 0)){
		std::vector<char> error_message(log_length+1);
		glGetShaderInfoLog(fragment_shader_id, log_length, NULL, &error_message[0]);
		LOG(ERROR) << &error_message[0];
		return 0;
	}

    LOG(INFO) << "Linking program...";
    
	GLuint program_id = glCreateProgram();
	glAttachShader(program_id, vertex_shader_id);
	glAttachShader(program_id, fragment_shader_id);
	glLinkProgram(program_id);

    glGetProgramiv(program_id, GL_LINK_STATUS, &result);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
	if ((result == GL_FALSE) && (log_length > 0)){
		std::vector<char> error_message(log_length+1);
		glGetProgramInfoLog(program_id, log_length, NULL, &error_message[0]);
		LOG(ERROR) << &error_message[0];
		return 0;
	}
	
	glValidateProgram(program_id);

    glGetProgramiv(program_id, GL_VALIDATE_STATUS, &result);
	glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);
	if ((result == GL_FALSE) && (log_length > 0)){
		std::vector<char> error_message(log_length+1);
		glGetProgramInfoLog(program_id, log_length, NULL, &error_message[0]);
		LOG(ERROR) << &error_message[0];
		return 0;
	}
	
	glDetachShader(program_id, vertex_shader_id);
	glDetachShader(program_id, fragment_shader_id);
	
	glDeleteShader(vertex_shader_id);
	glDeleteShader(fragment_shader_id);
	
	LOG(INFO) << "Success!";

	return program_id;
}

int print_error(int line)
{
    GLenum glErr;
    int    retCode = 0;

    glErr = glGetError();
    if (glErr != GL_NO_ERROR)
    {
        LOG(INFO) << line << " " << gluErrorString(glErr);
        retCode = 1;
    }
    return retCode;
}

int main(int argc, char * argv[]) try
{

    glfwInit();
    GLFWwindow * win = glfwCreateWindow(1280, 960, "main", 0, 0);
    glfwMakeContextCurrent(win);
	
	#ifdef WIN32
		// Initialize GLEW
		glewExperimental=TRUE;
		GLenum err = glewInit();
		if (err != GLEW_OK)
			LOG(ERROR) << "Could not initialize GLEW!";
	#endif

    // create root-level container for the GUI
	Panel c(".",{0,0},{1.0f, 1.0f},Alignment::left); 
	
	std::shared_ptr<Context> dcPlus(new Context(1));
	std::shared_ptr<Context> dcMinus(new Context(-1));
	
	try
	{
	    LOG(INFO) << "Loading UI...";
	    Serializer s("resources/ui.xml");
	    c.add_item(s.deserialize());
	    setup_ui(&c, dcPlus, dcMinus);
	    Rect origin { { 0, 0 }, { 1280, 960 } };
        c.arrange(origin);
        LOG(INFO) << "UI has been succesfully loaded and arranged";
	} catch(const std::exception& ex) {
	    LOG(ERROR) << "UI Loading Error: " << ex.what();
	    stringstream ss; ss << "UI Loading Error: \"" << ex.what() << "\"";
	    c.add_item(std::shared_ptr<TextBlock>(new TextBlock(
	            "txtError",
                ss.str(),
                Alignment::left,
                {0,0},
                {0,0},
                { 1.0f, 0.2f, 0.2f }
	        )));
	}
	
	Font f("resources/fonts/v.fnt");
	TextMesh m(f, "Hello World!");
	
	LOG(INFO) << m.get_width() << " x " << m.get_height();
	
	auto program_id = load_shaders("resources/shaders/font_vertex.c",
	                               "resources/shaders/font_fragment.c");
	print_error(__LINE__);

    glfwSetWindowUserPointer(win, &c);
    glfwSetCursorPosCallback(win, [](GLFWwindow * w, double x, double y) {
        /*auto ui_element = (IVisualElement*)glfwGetWindowUserPointer(w);
        ui_element->update_mouse_position({ (int)x, (int)y });*/
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
    
    static const GLfloat g_vertex_buffer_data[] = {
       -1.0f, -1.0f, 0.0f,
       1.0f, -1.0f, 0.0f,
       0.0f,  1.0f, 0.0f,
    };
    
    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, m.get_size(), m.get_vertex_positions(), GL_STATIC_DRAW);
    //glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);
    //glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
    
    /*glVertexAttribPointer(
       0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
       3,                  // size
       GL_FLOAT,           // type
       GL_FALSE,           // normalized?
       0,                  // stride
       (void*)0            // array buffer offset
    );*/
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    while (!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        int w,h;
        //glfwGetFramebufferSize(win, &w, &h);
        //glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        //glPushMatrix();
        glfwGetWindowSize(win, &w, &h);
        //glOrtho(0, w, h, 0, -1, +1);
        
        dcPlus->update();
        dcMinus->update();

        Rect origin { { 0, 0 }, { w, h } };

        //c.render(origin);
        //print_error(__LINE__);
		
        glUseProgram(program_id);
		print_error(__LINE__);
        
        /*glBegin(GL_QUADS);
        for (auto i = 0; i < m.get_vertex_count(); i++)
        {
            float x;
            float y;
            m.get_vertex(i, x, y);
            LOG(INFO) << "Vertex " << x << ", " << y;
            glVertex3f(x, y, 0);
        }
        glEnd();*/
        
        glBindVertexArray(vao);
		print_error(__LINE__);
        
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        
        glDrawArrays(GL_QUADS, 0, m.get_vertex_count());
		print_error(__LINE__);
        //glDrawElements(GL_QUADS, m.get_vertex_count(), GL_FLOAT, 0);

        glUseProgram(0);

        //glPopMatrix();
        glfwSwapBuffers(win);
    }

    glDeleteBuffers(1, &vbo);
    glDeleteBuffers(1, &vao);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
catch(const std::exception & e)
{
    LOG(ERROR) << "Program crashed! " << e.what();
    return -1;
}
