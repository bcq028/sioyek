#include "pdf_view_opengl_widget.h"

extern filesystem::path parent_path;
extern float background_color[3];

GLfloat g_quad_vertex[] = {
	-1.0f, -1.0f,
	1.0f, -1.0f,
	-1.0f, 1.0f,
	1.0f, 1.0f
};

GLfloat g_quad_uvs[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f
};

OpenGLSharedResources PdfViewOpenGLWidget::shared_gl_objects;

GLuint PdfViewOpenGLWidget::LoadShaders(filesystem::path vertex_file_path_, filesystem::path fragment_file_path_) {

	const wchar_t* vertex_file_path = vertex_file_path_.c_str();
	const wchar_t* fragment_file_path = fragment_file_path_.c_str();
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << VertexShaderStream.rdbuf();
		VertexShaderCode = sstr.str();
		VertexShaderStream.close();
	}
	else {
		wprintf(L"Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
		return 0;
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::stringstream sstr;
		sstr << FragmentShaderStream.rdbuf();
		FragmentShaderCode = sstr.str();
		FragmentShaderStream.close();
	}
	else {
		wprintf(L"Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", fragment_file_path);
		return 0;
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const* VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const* FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDetachShader(ProgramID, VertexShaderID);
	glDetachShader(ProgramID, FragmentShaderID);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

void PdfViewOpenGLWidget::initializeGL() {
	is_opengl_initialized = true;

	initializeOpenGLFunctions();

	if (!shared_gl_objects.is_initialized) {
		// we initialize the shared opengl objects here. Ideally we should have initialized them before any object
		// of this type was created but we could not use any OpenGL function before initalizeGL is called for the
		// first time.

		shared_gl_objects.is_initialized = true;

		shared_gl_objects.rendered_program = LoadShaders(parent_path / "shaders\\simple.vertex", parent_path / "shaders\\simple.fragment");
		shared_gl_objects.unrendered_program = LoadShaders(parent_path / "shaders\\simple.vertex", parent_path / "shaders\\unrendered_page.fragment");
		shared_gl_objects.highlight_program = LoadShaders(parent_path / "shaders\\simple.vertex", parent_path / "shaders\\highlight.fragment");

		shared_gl_objects.highlight_color_uniform_location = glGetUniformLocation(shared_gl_objects.highlight_program, "highlight_color");

		glGenBuffers(1, &shared_gl_objects.vertex_buffer_object);
		glGenBuffers(1, &shared_gl_objects.uv_buffer_object);

		glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex), g_quad_vertex, GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_uvs), g_quad_uvs, GL_DYNAMIC_DRAW);

	}

	//vertex array objects can not be shared for some reason!
	glGenVertexArrays(1, &vertex_array_object);
	glBindVertexArray(vertex_array_object);

	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.uv_buffer_object);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
}

void PdfViewOpenGLWidget::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);

	if (document_view) {
		document_view->on_view_size_change(w, h);
	}
}

void PdfViewOpenGLWidget::render_highlight_window(GLuint program, fz_rect window_rect) {

	float quad_vertex_data[] = {
		window_rect.x0, window_rect.y1,
		window_rect.x1, window_rect.y1,
		window_rect.x0, window_rect.y0,
		window_rect.x1, window_rect.y0
	};

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glUseProgram(program);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertex_data), quad_vertex_data, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glDisable(GL_BLEND);
}

void PdfViewOpenGLWidget::render_highlight_absolute(GLuint program, fz_rect absolute_document_rect) {
	fz_rect window_rect = document_view->absolute_to_window_rect(absolute_document_rect);
	render_highlight_window(program, window_rect);
}

void PdfViewOpenGLWidget::render_highlight_document(GLuint program, int page, fz_rect doc_rect) {
	fz_rect window_rect = document_view->document_to_window_rect(page, doc_rect);
	render_highlight_window(program, window_rect);
}

void PdfViewOpenGLWidget::paintGL() {

	glDisable(GL_CULL_FACE);
	glDisable(GL_BLEND);
	glBindVertexArray(vertex_array_object);
	render();
}

PdfViewOpenGLWidget::PdfViewOpenGLWidget(DocumentView* document_view, PdfRenderer* pdf_renderer, ConfigManager* config_manager, QWidget* parent) :
	QOpenGLWidget(parent),
	document_view(document_view),
	config_manager(config_manager),
	pdf_renderer(pdf_renderer)
{
}

void PdfViewOpenGLWidget::handle_escape() {
	search_results.clear();
	current_search_result_index = 0;
	is_searching = false;
	is_search_cancelled = true;
}

void PdfViewOpenGLWidget::toggle_highlight_links() {
	this->should_highlight_links = !this->should_highlight_links;
}

int PdfViewOpenGLWidget::get_num_search_results() {
	search_results_mutex.lock();
	int num = search_results.size();
	search_results_mutex.unlock();
	return num;
}

int PdfViewOpenGLWidget::get_current_search_result_index() {
	return current_search_result_index;
}

bool PdfViewOpenGLWidget::valid_document() {
	if (document_view) {
		if (document_view->get_document()) {
			return true;
		}
	}
	return false;
}

void PdfViewOpenGLWidget::goto_search_result(int offset) {
	if (!valid_document()) return;

	search_results_mutex.lock();
	if (search_results.size() == 0) {
		search_results_mutex.unlock();
		return;
	}

	int target_index = mod(current_search_result_index + offset, search_results.size());
	current_search_result_index = target_index;

	auto [rect, target_page] = search_results[target_index];
	search_results_mutex.unlock();

	float new_offset_y = rect.y0 + document_view->get_document()->get_accum_page_height(target_page);

	document_view->set_offset_y(new_offset_y);
}

void PdfViewOpenGLWidget::render_page(int page_number) {

	if (!valid_document()) return;

	GLuint texture = pdf_renderer->find_rendered_page(document_view->get_document()->get_path(),
		page_number,
		document_view->get_zoom_level(),
		nullptr,
		nullptr);

	float page_vertices[4 * 2];
	fz_rect page_rect = { 0,
		0,
		document_view->get_document()->get_page_width(page_number),
		document_view->get_document()->get_page_height(page_number) };

	fz_rect window_rect = document_view->document_to_window_rect(page_number, page_rect);
	rect_to_quad(window_rect, page_vertices);

	if (texture != 0) {

		glUseProgram(shared_gl_objects.rendered_program);
		glBindTexture(GL_TEXTURE_2D, texture);
	}
	else {
		glUseProgram(shared_gl_objects.unrendered_program);
	}
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, shared_gl_objects.vertex_buffer_object);
	glBufferData(GL_ARRAY_BUFFER, sizeof(page_vertices), page_vertices, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void PdfViewOpenGLWidget::render() {
	if (!valid_document()) {
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		return;
	}

	vector<int> visible_pages;
	document_view->get_visible_pages(document_view->get_view_height(), visible_pages);

	glClearColor(background_color[0], background_color[1], background_color[2], 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);


	for (int page : visible_pages) {
		render_page(page);

		if (should_highlight_links) {
			glUseProgram(shared_gl_objects.highlight_program);
			glUniform3fv(shared_gl_objects.highlight_color_uniform_location,
				1,
				config_manager->get_config<float>(L"link_highlight_color"));
			fz_link* links = document_view->get_document()->get_page_links(page);
			while (links != nullptr) {
				render_highlight_document(shared_gl_objects.highlight_program, page, links->rect);
				links = links->next;
			}
		}
	}

	search_results_mutex.lock();
	if (search_results.size() > 0) {
		SearchResult current_search_result = search_results[current_search_result_index];
		glUseProgram(shared_gl_objects.highlight_program);
		glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, config_manager->get_config<float>(L"search_highlight_color"));
		//glUniform3fv(g_shared_resources.highlight_color_uniform_location, 1, highlight_color_temp);
		render_highlight_document(shared_gl_objects.highlight_program, current_search_result.page, current_search_result.rect);
	}
	search_results_mutex.unlock();

	glUseProgram(shared_gl_objects.highlight_program);
	glUniform3fv(shared_gl_objects.highlight_color_uniform_location, 1, config_manager->get_config<float>(L"text_highlight_color"));
	for (auto rect : selected_character_rects) {
		render_highlight_absolute(shared_gl_objects.highlight_program, rect);
	}
}

bool PdfViewOpenGLWidget::get_is_searching(float* prog)
{
	if (is_search_cancelled) {
		return false;
	}

	search_results_mutex.lock();
	bool res = is_searching;
	if (is_searching) {
		*prog = percent_done;
	}
	search_results_mutex.unlock();
	return res;
}

void PdfViewOpenGLWidget::search_text(const wchar_t* text) {
	if (!document_view) return;

	search_results_mutex.lock();
	search_results.clear();
	current_search_result_index = 0;
	search_results_mutex.unlock();

	is_searching = true;
	is_search_cancelled = false;
	pdf_renderer->add_request(
		document_view->get_document()->get_path(),
		document_view->get_current_page_number(),
		text,
		&search_results,
		&percent_done,
		&is_searching,
		&search_results_mutex);
}

PdfViewOpenGLWidget::~PdfViewOpenGLWidget() {
	if (is_opengl_initialized) {
		glDeleteVertexArrays(1, &vertex_array_object);
	}
}