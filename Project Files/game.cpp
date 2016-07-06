#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <FTGL/ftgl.h>
#include <SOIL/SOIL.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace std;

double last_update_time, current_time;


struct VAO {
    GLuint VertexArrayID;
    GLuint VertexBuffer;
    GLuint ColorBuffer;
    GLuint TextureBuffer;
	GLuint TextureID;
    GLenum PrimitiveMode;
    GLenum FillMode;
    int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

struct FTGLFont {
	FTFont* font;
	GLuint fontMatrixID;
	GLuint fontColorID;
} GL3Font;

GLuint programID, fontProgramID, textureProgramID;

/* Function to load Shaders - Use it as it is */
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
	if(VertexShaderStream.is_open())
	{
		std::string Line = "";
		while(getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
	if(FragmentShaderStream.is_open()){
		std::string Line = "";
		while(getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_file_path);
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> VertexShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_file_path);
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
	glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
	fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

	// Link the program
	fprintf(stdout, "Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);
	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
	glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
	fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return ProgramID;
}

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

void quit(GLFWwindow *window)
{
    glfwDestroyWindow(window);
    glfwTerminate();
    exit(EXIT_SUCCESS);
}

glm::vec3 getRGBfromHue (int hue)
{
	float intp;
	float fracp = modff(hue/60.0, &intp);
	float x = 1.0 - abs((float)((int)intp%2)+fracp-1.0);

	if (hue < 60)
		return glm::vec3(1,x,0);
	else if (hue < 120)
		return glm::vec3(x,1,0);
	else if (hue < 180)
		return glm::vec3(0,1,x);
	else if (hue < 240)
		return glm::vec3(0,x,1);
	else if (hue < 300)
		return glm::vec3(x,0,1);
	else
		return glm::vec3(1,0,x);
}


/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
    struct VAO* vao = new struct VAO;
    vao->PrimitiveMode = primitive_mode;
    vao->NumVertices = numVertices;
    vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
}

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
{
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
        color_buffer_data [3*i] = red;
        color_buffer_data [3*i + 1] = red;
        color_buffer_data [3*i + 2] = red;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
}

struct VAO* create3DTexturedObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* texture_buffer_data, GLuint textureID, GLenum fill_mode=GL_FILL)
{
	struct VAO* vao = new struct VAO;
	vao->PrimitiveMode = primitive_mode;
	vao->NumVertices = numVertices;
	vao->FillMode = fill_mode;
	vao->TextureID = textureID;

	// Create Vertex Array Object
	// Should be done after CreateWindow and before any other GL calls
	glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
	glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
	glGenBuffers (1, &(vao->TextureBuffer));  // VBO - textures

	glBindVertexArray (vao->VertexArrayID); // Bind the VAO
	glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices
	glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
	glVertexAttribPointer(
						  0,                  // attribute 0. Vertices
						  3,                  // size (x,y,z)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	glBindBuffer (GL_ARRAY_BUFFER, vao->TextureBuffer); // Bind the VBO textures
	glBufferData (GL_ARRAY_BUFFER, 2*numVertices*sizeof(GLfloat), texture_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
	glVertexAttribPointer(
						  2,                  // attribute 2. Textures
						  2,                  // size (s,t)
						  GL_FLOAT,           // type
						  GL_FALSE,           // normalized?
						  0,                  // stride
						  (void*)0            // array buffer offset
						  );

	return vao;
}



/* Render the VBOs handled by VAO */
void draw3DObject (struct VAO* vao)
{
    // Change the Fill Mode for this object
    glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
    glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
    glEnableVertexAttribArray(0);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
    glEnableVertexAttribArray(1);
    // Bind the VBO to use
    glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
}
void draw3DTexturedObject (struct VAO* vao)
{
	// Change the Fill Mode for this object
	glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

	// Bind the VAO to use
	glBindVertexArray (vao->VertexArrayID);

	// Enable Vertex Attribute 0 - 3d Vertices
	glEnableVertexAttribArray(0);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

	// Bind Textures using texture units
	glBindTexture(GL_TEXTURE_2D, vao->TextureID);

	// Enable Vertex Attribute 2 - Texture
	glEnableVertexAttribArray(2);
	// Bind the VBO to use
	glBindBuffer(GL_ARRAY_BUFFER, vao->TextureBuffer);

	// Draw the geometry !
	glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle

	// Unbind Textures to be safe
	glBindTexture(GL_TEXTURE_2D, 0);
}

/* Create an OpenGL Texture from an image 
GLuint createTexture (const char* filename)
{
	GLuint TextureID;
	// Generate Texture Buffer
	glGenTextures(1, &TextureID);
	// All upcoming GL_TEXTURE_2D operations now have effect on our texture buffer
	glBindTexture(GL_TEXTURE_2D, TextureID);
	// Set our texture parameters
	// Set texture wrapping to GL_REPEAT
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// Set texture filtering (interpolation)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Load image and create OpenGL texture
	int twidth, theight;
	unsigned char* image = SOIL_load_image(filename, &twidth, &theight, 0, SOIL_LOAD_RGB);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, twidth, theight, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D); // Generate MipMaps to use
	SOIL_free_image_data(image); // Free the data read from file after creating opengl texture
	glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess it up

	return TextureID;
}

*/

/**************************
 * Customizable functions *
 **************************/

/*float triangle_rot_dir = 1;
float rectangle_rot_dir = 1;
bool triangle_rot_status = true;
bool rectangle_rot_status = true;*/
float u = 4.0;
float g = 4.0;
float thita = 45;
float thita_ball = 45;
int fire = 0;
int fl = 0;
float u_xn = -4.0f;
float u_xp = 4.0f;
float u_yn = -4.0f;
float u_yp = 4.0f;
float camera_position = 0.0f;
void zoomin()
{
	u_xn += 0.25f;
	u_xp -= 0.25f;
	u_yn += 0.125f;
	u_yp -= 0.125f;
	Matrices.projection = glm::ortho(u_xn, u_xp, u_yn, u_yp, 0.1f, 500.0f);
}
void zoomout()
{
	u_xn -= 0.25f;
	u_xp += 0.25f;
	u_yn -= 0.125f;
	u_yp += 0.125f;
	Matrices.projection = glm::ortho(u_xn, u_xp, u_yn, u_yp, 0.1f, 500.0f);	
}
void panleft()
{
	camera_position += 0.1f;
}
void panright()
{
	camera_position -= 0.1f;
}
/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
{
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
        switch (key) {
            case GLFW_KEY_A:
                thita += 2;
                break;

            case GLFW_KEY_UP:
            	zoomin();
            	break;

            case GLFW_KEY_DOWN:
            	zoomout();
            	break; 
            case GLFW_KEY_LEFT:
            	panleft();
            	break;
            case GLFW_KEY_RIGHT:
            	panright();
            	break;
            case GLFW_KEY_B:
                thita -= 2;
                break;

            case GLFW_KEY_SPACE:
            {
            	thita_ball = thita;
                fire = 1;
                fl = 1;
                last_update_time = glfwGetTime();
	            break;

	        }

            case GLFW_KEY_F:
            	u += 0.2;
            	break;

            case GLFW_KEY_S:
            	u -= 0.2;
            	break;

            default:
                break;
        }
    }
    else if (action == GLFW_REPEAT) {
        switch (key) {
            case GLFW_KEY_A:
                thita += 5;
                
                break;
            case GLFW_KEY_B:
                thita -= 5;
                break;

            case GLFW_KEY_UP:
            	zoomin();
            	break;

            case GLFW_KEY_DOWN:
            	zoomout();
            	break; 
            case GLFW_KEY_LEFT:
            	panleft();
            	break;
            case GLFW_KEY_RIGHT:
            	panright();
            	break;
            case GLFW_KEY_SPACE:
                break;

            case GLFW_KEY_F:
            	u += 0.1;
            	break;

            case GLFW_KEY_S:
            	u -= 0.1;
            	break;

            default:
                break;
        }
    }
    else if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_ESCAPE:
                quit(window);
                break;
            default:
                break;
        }
    }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
            quit(window);
            break;
		default:
			break;
	}
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
    switch (button) {
        case GLFW_MOUSE_BUTTON_LEFT:
            if (action == GLFW_RELEASE)
                //triangle_rot_dir *= -1;
            break;
        case GLFW_MOUSE_BUTTON_RIGHT:
            if (action == GLFW_RELEASE) {
                //rectangle_rot_dir *= -1;
            }
            break;
        default:
            break;
    }
}

/* Executed when window is resized to 'width' and 'height' */
/* Modify the bounds of the screen here in glm::ortho or Field of View in glm::Perspective */
void reshapeWindow (GLFWwindow* window, int width, int height)
{
    int fbwidth=width, fbheight=height;
    /* With Retina display on Mac OS X, GLFW's FramebufferSize
     is different from WindowSize */
    glfwGetFramebufferSize(window, &fbwidth, &fbheight);

	GLfloat fov = 90.0f;

	// sets the viewport of openGL renderer
	glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

	// set the projection matrix as perspective
	/* glMatrixMode (GL_PROJECTION);
	   glLoadIdentity ();
	   gluPerspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1, 500.0); */
	// Store the projection matrix in a variable for future use
    // Perspective projection for 3D views
    // Matrices.projection = glm::perspective (fov, (GLfloat) fbwidth / (GLfloat) fbheight, 0.1f, 500.0f);

    // Ortho projection for 2D views
    Matrices.projection = glm::ortho(u_xn, u_xp, u_yn, u_yp, 0.1f, 500.0f);
}

VAO *triangle, *rectangle1, *rectangle2, *trep, *cannon_circle, *cannon_ball, *zameen, *obs1, *obs2_1, *obs2_2, *obs3;

// Creates the triangle object used in this sample code
/*void createTriangle ()
{
  /* ONLY vertices between the bounds specified in glm::ortho will be visible on screen */

  /* Define vertex array as used in glBegin (GL_TRIANGLES) 
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_LINE);
}*/
void createTrep()
{
	static const GLfloat vertex_buffer_data [] = {
    0.3,0,0,
    0.15,1,0,
    0,0,0,

    0.15,1,0,
    0,0,0,
    -0.15,1,0,

    0,0,0,
    -0.15,1,0,
    -0.3,0,0
  };

  static const GLfloat color_buffer_data [] = {
    0,0,0, // color 1
    0,0,0, // color 2
    0,0,0, // color 3

    0,0,0, // color 3
    0,0,0, // color 4
    0,0,0,  // color 1

    0,0,0, // color 3
    0,0,0, // color 4
    0,0,0  // color 1
  };
  trep = create3DObject(GL_TRIANGLES, 9, vertex_buffer_data, color_buffer_data, GL_FILL);
}

float TwoPI = 2 * 3.14159;
void createCannonCircle()
{
	int NoOfTriangles = 360;
	int i;
	float radius = 0.3;
	GLfloat vertex_buffer_data[3240];
	GLfloat color_buffer_data[3240];
	for(i=0 ; i<NoOfTriangles ; i++)
	{
		vertex_buffer_data[9*i] = 0;
		vertex_buffer_data[9*i+1] = 0;
		vertex_buffer_data[9*i+2] = 0;	
		
		vertex_buffer_data [9*i+3] = radius * cos(i * TwoPI / NoOfTriangles);
		vertex_buffer_data [9*i+4] = radius * sin(i * TwoPI / NoOfTriangles);
		vertex_buffer_data [9*i+5] = 0;

		vertex_buffer_data [9*i+6] = radius * cos((i+1) * TwoPI / NoOfTriangles);
		vertex_buffer_data [9*i+7] = radius * sin((i+1) * TwoPI / NoOfTriangles);
		vertex_buffer_data [9*i+8] = 0;
	}
	for(i=0 ; i<NoOfTriangles ; i++)
	{
		color_buffer_data[9*i] = 0;
		color_buffer_data[9*i+1] = 0;
		color_buffer_data[9*i+2] = 0;

		color_buffer_data[9*i+3] = 0;
		color_buffer_data[9*i+4] = 0;
		color_buffer_data[9*i+5] = 0;

		color_buffer_data[9*i+6] = 0;
		color_buffer_data[9*i+7] = 0;
		color_buffer_data[9*i+8] = 0;

	}

	cannon_circle = create3DObject(GL_TRIANGLES, 1080 , vertex_buffer_data, color_buffer_data, GL_FILL);
}

void createCannonBall()
{
	int NoOfTriangles = 360;
	int i;
	float radius = 0.15;
	GLfloat vertex_buffer_data[3240];
	GLfloat color_buffer_data[3240];
	for(i=0 ; i<NoOfTriangles ; i++)
	{
		vertex_buffer_data[9*i] = 0;
		vertex_buffer_data[9*i+1] = 0;
		vertex_buffer_data[9*i+2] = 0;	
		
		vertex_buffer_data [9*i+3] = radius * cos(i * TwoPI / NoOfTriangles);
		vertex_buffer_data [9*i+4] = radius * sin(i * TwoPI / NoOfTriangles);
		vertex_buffer_data [9*i+5] = 0;

		vertex_buffer_data [9*i+6] = radius * cos((i+1) * TwoPI / NoOfTriangles);
		vertex_buffer_data [9*i+7] = radius * sin((i+1) * TwoPI / NoOfTriangles);
		vertex_buffer_data [9*i+8] = 0;
	}
	for(i=0 ; i<NoOfTriangles ; i++)
	{
		color_buffer_data[9*i] = 0;
		color_buffer_data[9*i+1] = 0;
		color_buffer_data[9*i+2] = 0;

		color_buffer_data[9*i+3] = 0;
		color_buffer_data[9*i+4] = 0;
		color_buffer_data[9*i+5] = 0;

		color_buffer_data[9*i+6] = 0;
		color_buffer_data[9*i+7] = 0;
		color_buffer_data[9*i+8] = 0;

	}

	cannon_ball = create3DObject(GL_TRIANGLES, 1080 , vertex_buffer_data, color_buffer_data, GL_FILL);
}
// Creates the rectangle object used in this sample code

void CreateTarget1()
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		0.5,0,0, // vertex 1
		0.5,1,0, // vertex 2
		-0.5,1,0, // vertex 3

		-0.5,1,0, // vertex 3
		-0.5,0,0, // vertex 4
		0.5,0,0  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0,0,0, // color 1
		0,0,0, // color 2
		0,0,0, // color 3

		0,0,0, // color 3
		0,0,0, // color 4
		0,0,0  // color 1
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	rectangle1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void CreateTarget2()
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		0.4,0,0, // vertex 1
		0.4,1,0, // vertex 2
		-0.4,1,0, // vertex 3

		-0.4,1,0, // vertex 3
		-0.4,0,0, // vertex 4
		0.4,0,0  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0,0,0, // color 1
		0,0,0, // color 2
		0,0,0, // color 3

		0,0,0, // color 3
		0,0,0, // color 4
		0,0,0  // color 1
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	rectangle2 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createObs2_1 ()
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		0.2,1.0f,0, // vertex 1
		-0.2,1.0f,0, // vertex 2
		-0.2,-1,0, // vertex 3

		-0.2,-1,0, // vertex 3
		0.2,-1,0, // vertex 4
		0.2,1.0f,0  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0.4f,0.6f,0.6f, // color 1
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	obs2_1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createObs2_2()
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		0.2,1.0f,0, // vertex 1
		-0.2,1.0f,0, // vertex 2
		-0.2,-1,0, // vertex 3

		-0.2,-1,0, // vertex 3
		0.2,-1,0, // vertex 4
		0.2,1.0f,0  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0.4f,0.6f,0.6f, // color 1
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	obs2_2 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createObs1()
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		0.2,1.5,0, // vertex 1
		-0.2,1.5,0, // vertex 2
		-0.2,-2,0, // vertex 3

		-0.2,-2,0, // vertex 3
		0.2,-2,0, // vertex 4
		0.2,1.5,0  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0.4f,0.6f,0.6f, // color 1
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	obs1 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void createObs3()
{
	// GL3 accepts only Triangles. Quads are not supported
	static const GLfloat vertex_buffer_data [] = {
		0.2,2,0, // vertex 1
		-0.2,2,0, // vertex 2
		-0.2,-2,0, // vertex 3

		-0.2,-2,0, // vertex 3
		0.2,-2,0, // vertex 4
		0.2,2,0  // vertex 1
	};

	static const GLfloat color_buffer_data [] = {
		0.4f,0.6f,0.6f, // color 1
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f,
		0.4f,0.6f,0.6f
	};

	// create3DObject creates and returns a handle to a VAO that can be used later
	obs3 = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}


void createFloor ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    4,0.3,0, // vertex 1
    -4,0.3,0, // vertex 2
    -4,-0.8,0, // vertex 3

    -4,-0.8,0, // vertex 3
    4,-0.8,0, // vertex 4
    4,0.3,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    0,0.51,0, // color 1
    0,0.51,0, // color 2
    0,0.51,0, // color 3

    0,0.51,0, // color 3
    0,0.51,0, // color 4
    0,0.51,0  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  zameen = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}

float camera_rotation_angle = 90;
float rectangle_rotation = 0;
float triangle_rotation = 0;
float t = 0;
int difficulty_level  = 1;
int Target_visible = 1;
float x_cannonball;
float y_cannonball;
float e = 0.6;
float v = u; 
float ux;
float uy;
float vx = ux;
float vy;
int score = 0;
int ball_visible = 1;
int collision_flag = 0;
int in_air_flag = 0;
float x_till_collision = -3.0f;
float y_till_collision = -2.75f;
float delta;
float alpha;
float beta;
int obs_collision = 0;
float t_till_now = 0;
//int updatescore = 0 ;

//if(updatescore == 1)
//{
//	score += 5;
//}

bool checkCollisionTarget(float x_ball, float y_ball, float xsmall_target, float xlarge_target, float ysmall_target, float ylarge_target)
{
	if(x_ball >= xsmall_target && x_ball <= xlarge_target && y_ball <= ylarge_target && y_ball >= ysmall_target)
		return true;
	else
		return false;
}

void CheckFloorCollisions(float x_ball, float y_ball, float x_small, float x_large,float y_small)
{
	if(y_ball >= y_small && y_ball < (y_small + 0.15f) && x_ball < x_large && x_ball > x_small)
	{	
		///cout << "collide with floor!\n";
		cout << "thita ball collide with floor" << thita_ball << " thita: " << thita << "\n";
		collision_flag = 1;
		in_air_flag = 0;
		fire = 0;
		x_till_collision = x_ball;
		y_till_collision = y_ball + 0.1f;
		//u = inst_velocty;
		obs_collision = 0;
		t_till_now = 0;
	}
}

bool checkCollisionObs(float x_ball, float y_ball,  float xsmall_obs, float xlarge_obs, float ysmall_obs, float ylarge_obs, float t)
{
	///cout << "x_ball: " << x_ball << "\n";
	///cout << "y_ball: " << y_ball << "\n" ;
	if(x_ball >= (xsmall_obs - 0.15f) && x_ball < (xlarge_obs + 0.15f) && y_ball < ylarge_obs && y_ball > ysmall_obs)
	{
		///cout << "Collide with Obs\n";
		cout << "thita ball collide with OBS: " << thita_ball << " thita: " << thita << "\n";
		//cout << "dist bw centers:" << sqrt(pow((x_ball - x_centroid),2) + pow((y_ball - y_centroid),2)) << "<" << "radius + 0.15: "<< 0.15f + radius << "\n";
		collision_flag = 1;
		in_air_flag = 0;
		fire = 0;
		if(vx > 0)
		{
			x_till_collision = x_ball - 0.1f;
		}
		else
		{
			x_till_collision = x_ball + 0.1f;
		}
		//y_till_collision = y_ball;
		t_till_now = t;
		//alpha = acos((float)( (u * cos((float)((thita_ball+90)*M_PI/180.0f))) / inst_velocty));
		//delta = atan((float)((0 - x_till_collision)/(-2.0f - y_till_collision)));
		//u = inst_velocty;
		obs_collision = 1;
		return true;
	}
	return false;
}

/* Render the scene with openGL */
/* Edit this function according to your assignment */
void drawCannon () //Draws cannon plus fireballs
{
  
  
  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(camera_position,0,3), glm::vec3(camera_position,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model
/*
 // Load identity to model matrix
  Matrices.model = glm::mat4(1.0f);

  /* Render your scene 

  glm::mat4 translateTriangle = glm::translate (glm::vec3(-2.0f, 0.0f, 0.0f)); // glTranslatef
  glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
  Matrices.model *= triangleTransform; 
  MVP = VP * Matrices.model; // MVP = p * V * M

  //  Don't change unless you are sure!!
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);*/

  // draw3DObject draws the VAO given to it using current MVP matrix
  //draw3DObject(triangle);

  // Pop matrix to undo transformations till last push matrix instead of recomputing model matrix
  // glPopMatrix ();

	
  Matrices.model = glm::mat4(1.0f);

  glm::mat4 translateTrep = glm::translate (glm::vec3(-3,-2.75,0));        // glTranslatef
  glm::mat4 rotateTrep = glm::rotate((float)((thita-90) * M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateTrep * rotateTrep);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix,
  draw3DObject(trep);

  

//***** end of exp code *****//
   
   Matrices.model = glm::mat4(1.0f);


  glm::mat4 translateCannonCircle = glm::translate (glm::vec3(-3,-2.75,0));        // glTranslatef
  //glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= translateCannonCircle;
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

  // draw3DObject draws the VAO given to it using current MVP matrix
  draw3DObject(cannon_circle);
}
void drawCannonBall(float x_ball, float y_ball)
{
	 // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
Matrices.view = glm::lookAt(glm::vec3(camera_position,0,3), glm::vec3(camera_position,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model
  Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateCannonBall = glm::translate (glm::vec3(x_ball, y_ball,0));        // glTranslatef

	Matrices.model *= translateCannonBall;
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
    // draw3DObject draws the VAO given to it using current MVP matrix
    draw3DObject(cannon_ball);
}

void drawFloor(){  
	// clear the color and depth in the frame buffer
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
  Matrices.view = glm::lookAt(glm::vec3(camera_position,0,3), glm::vec3(camera_position,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

    glm::mat4 MVP;	// MVP = Projection * View * Model

   Matrices.model = glm::mat4(1.0f);

	  glm::mat4 translateFloor = glm::translate (glm::vec3(0,-3.3,0));        // glTranslatef
	  //glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	  Matrices.model *= (translateFloor);
 	  MVP = VP * Matrices.model;
	  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	  // draw3DObject draws the VAO given to it using current MVP matrix
	  draw3DObject(zameen);

	  // Render font on screen
	static int fontScale = 0;
	float fontScaleValue = 0.5f;
	glm::vec3 fontColor = getRGBfromHue (fontScale);



	// Use font Shaders for next part of code
	glUseProgram(fontProgramID);
	Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Transform the text
	Matrices.model = glm::mat4(1.0f);
	glm::mat4 translateText = glm::translate(glm::vec3(-4.0f,3,0));
	glm::mat4 scaleText = glm::scale(glm::vec3(fontScaleValue,fontScaleValue,fontScaleValue));
	Matrices.model *= (translateText * scaleText);
	MVP = Matrices.projection * Matrices.view * Matrices.model;
	// send font's MVP and font color to fond shaders
	glUniformMatrix4fv(GL3Font.fontMatrixID, 1, GL_FALSE, &MVP[0][0]);
	glUniform3fv(GL3Font.fontColorID, 1, &fontColor[0]);
	char S[1000];
	sprintf(S,"Initial Velocity: %.3f Difficulty level: %d Score: %d",u,difficulty_level,score);
	// Render font
	GL3Font.font->Render(S);

	// font size and color changes
	//fontScale = (fontScale + 1) % 360;
}
void drawObs1(){

  // use the loaded shader program
  // Don't change unless you know what you are doing
  glUseProgram (programID);

  // Eye - Location of camera. Don't change unless you are sure!!
  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  // Compute Camera matrix (view)
  // Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
  //  Don't change unless you are sure!!
Matrices.view = glm::lookAt(glm::vec3(camera_position,0,3), glm::vec3(camera_position,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  // Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
  //  Don't change unless you are sure!!
  glm::mat4 VP = Matrices.projection * Matrices.view;

  // Send our transformation to the currently bound shader, in the "MVP" uniform
  // For each model you render, since the MVP will be different (at least the M part)
  //  Don't change unless you are sure!!
  glm::mat4 MVP;	// MVP = Projection * View * Model

  Matrices.model = glm::mat4(1.0f);

	  glm::mat4 translateFloor = glm::translate (glm::vec3(-1,-3,0));        // glTranslatef
	  //glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	  Matrices.model *= (translateFloor);
 	  MVP = VP * Matrices.model;
	  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	  // draw3DObject draws the VAO given to it using current MVP matrix
	  draw3DObject(obs1);

}

void drawObs2_1(){

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	// Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	Matrices.view = glm::lookAt(glm::vec3(camera_position,0,3), glm::vec3(camera_position,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model

	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateObs2_1 = glm::translate (glm::vec3(-2.0f,-3.0f,0));        // glTranslatef
	//glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateObs2_1);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(obs2_1);

}
void drawObs2_2(){

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	// Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	Matrices.view = glm::lookAt(glm::vec3(camera_position,0,3), glm::vec3(camera_position,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model

	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateObs2_2 = glm::translate (glm::vec3(1.0f,-3.0f,0));        // glTranslatef
	//glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateObs2_2);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(obs2_2);

}
void drawObs3(){

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	// Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	Matrices.view = glm::lookAt(glm::vec3(camera_position,0,3), glm::vec3(camera_position,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model

	Matrices.model = glm::mat4(1.0f);

	glm::mat4 translateObs3 = glm::translate (glm::vec3(0,-3.0f,0));        // glTranslatef
	//glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
	Matrices.model *= (translateObs3);
	MVP = VP * Matrices.model;
	glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

	// draw3DObject draws the VAO given to it using current MVP matrix
	draw3DObject(obs3);

}
void drawTarget1(){
	// clear the color and depth in the frame buffer
	//glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	// Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	Matrices.view = glm::lookAt(glm::vec3(camera_position,0,3), glm::vec3(camera_position,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model

	if(!checkCollisionTarget(x_cannonball, y_cannonball, 1.5f, 2.5f, -3.0f, -2.0f) && Target_visible == 1)
	{
		Matrices.model = glm::mat4(1.0f);

		glm::mat4 translateTarget1 = glm::translate (glm::vec3(2,-3,0));        // glTranslatef
		//glm::mat4 rote = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
		Matrices.model *= (translateTarget1);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// draw3DObject draws the VAO given to it using current MVP matrix
		draw3DObject(rectangle1);

	}
	//else if()
	//{

	//}

	else 
	{
		Target_visible = 0;
		//updatescore = 1;
	}

}
void drawTarget2(){
	// clear the color and depth in the frame buffer
	//glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// use the loaded shader program
	// Don't change unless you know what you are doing
	glUseProgram (programID);

	// Eye - Location of camera. Don't change unless you are sure!!
	glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
	// Target - Where is the camera looking at.  Don't change unless you are sure!!
	glm::vec3 target (0, 0, 0);
	// Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
	glm::vec3 up (0, 1, 0);

	// Compute Camera matrix (view)
	// Matrices.view = glm::lookAt( eye, target, up ); // Rotating Camera for 3D
	//  Don't change unless you are sure!!
	Matrices.view = glm::lookAt(glm::vec3(camera_position,0,3), glm::vec3(camera_position,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

	// Compute ViewProject matrix as view/camera might not be changed for this frame (basic scenario)
	//  Don't change unless you are sure!!
	glm::mat4 VP = Matrices.projection * Matrices.view;

	// Send our transformation to the currently bound shader, in the "MVP" uniform
	// For each model you render, since the MVP will be different (at least the M part)
	//  Don't change unless you are sure!!
	glm::mat4 MVP;	// MVP = Projection * View * Model

	if(!checkCollisionTarget(x_cannonball, y_cannonball, 1.6f, 2.4f, -2.0f, -1.0f) && Target_visible == 1)
	{
		Matrices.model = glm::mat4(1.0f);

		glm::mat4 translateTarget1 = glm::translate (glm::vec3(2,-2,0));        // glTranslatef
		//glm::mat4 rote = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
		Matrices.model *= (translateTarget1);
		MVP = VP * Matrices.model;
		glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);

		// draw3DObject draws the VAO given to it using current MVP matrix
		draw3DObject(rectangle2);
	}
	else 
	{
		Target_visible = 0;
		
	}

}
/* Initialise glfw window, I/O callbacks and the renderer to use */
/* Nothing to Edit here */
GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Angry Birds >.<", NULL, NULL);

    if (!window) {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    /* --- register callbacks with GLFW --- */

    /* Register function to handle window resizes */
    /* With Retina display on Mac OS X GLFW's FramebufferSize
     is different from WindowSize */
    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    /* Register function to handle window close */
    glfwSetWindowCloseCallback(window, quit);

    /* Register function to handle keyboard input */
    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    /* Register function to handle mouse click */
    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
}

/* Initialize the OpenGL rendering properties */
/* Add all the models to be created here */
void initGL (GLFWwindow* window, int width, int height)
{
    /* Objects should be created before any other gl function and shaders */
	// Create the models
	//createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	CreateTarget1();
	CreateTarget2();
	createTrep();
	createCannonCircle();
	createCannonBall();
	createFloor();
	createObs1();
	createObs2_1();
	createObs2_2();
	createObs3();
	// Create and compile our GLSL program from the shaders
	programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
	// Get a handle for our "MVP" uniform
	Matrices.MatrixID = glGetUniformLocation(programID, "MVP");

	
	reshapeWindow (window, width, height);

    // Background color of the scene
	glClearColor (1.0f, 0.6f, 0.4f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	//glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

	// Initialise FTGL stuff
	const char* fontfile = "arial.ttf";
	GL3Font.font = new FTExtrudeFont(fontfile); // 3D extrude style rendering

	if(GL3Font.font->Error())
	{
		cout << "Error: Could not load font `" << fontfile << "'" << endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// Create and compile our GLSL program from the font shaders
	fontProgramID = LoadShaders( "fontrender.vert", "fontrender.frag" );
	GLint fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform;
	fontVertexCoordAttrib = glGetAttribLocation(fontProgramID, "vertexPosition");
	fontVertexNormalAttrib = glGetAttribLocation(fontProgramID, "vertexNormal");
	fontVertexOffsetUniform = glGetUniformLocation(fontProgramID, "pen");
	GL3Font.fontMatrixID = glGetUniformLocation(fontProgramID, "MVP");
	GL3Font.fontColorID = glGetUniformLocation(fontProgramID, "fontColor");

	GL3Font.font->ShaderLocations(fontVertexCoordAttrib, fontVertexNormalAttrib, fontVertexOffsetUniform);
	GL3Font.font->FaceSize(1);
	GL3Font.font->Depth(0);
	GL3Font.font->Outset(0, 0);
	GL3Font.font->CharMap(ft_encoding_unicode);

    cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
    cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
    cout << "VERSION: " << glGetString(GL_VERSION) << endl;
    cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 1000;
	int height = 1000;

    GLFWwindow* window = initGLFW(width, height);

	initGL (window, width, height);

    last_update_time = glfwGetTime();
    ux = u * cos((float)((thita_ball)*M_PI/180.0f));
		uy = u * sin((float)((thita_ball)*M_PI/180.0f));
    vx = ux;
    /* Draw in loop */
    while (!glfwWindowShouldClose(window)) 
    {
    	if(fl == 1)
    	{
    		//thita_ball = thita;
    		vx *= sqrt(2)*cos((float)((thita_ball)*M_PI/180.0f));
    		fl =0;
    	}
    	x_cannonball = x_till_collision + vx * (t-t_till_now); 
		y_cannonball = y_till_collision + u * sin((float)((thita_ball)*M_PI/180.0f))*(t) - 0.5f*g*(t)*(t);
		
		//cout << "x till collision: " << x_till_collision << "\n";
		///cout << "vx: " << vx << "\n";
		///cout << "t: "<< t << "\n";
		///cout << "x_till_collision: " << x_till_collision << "\n";
		//cout << "t : " << t << "\n";
		//cout << "ux : " << ux << "\n";
		//cout << "u : " << u << "\n";
    	//cout << "vy : " << vy << "\n";
    	//cout << "uy : " << uy << "\n";	
		///cout << "thita ball Before LOOP" << thita_ball << " thita: " << thita<< "\n";
		
		vy = uy - g*t;
		v = sqrt(pow(vx,2)+pow(vy,2)); 
		///cout << "v: " << v << "\n";
		//beta = atan((float) (e * tan(alpha + delta)));
		//***** cannon Ball *****// 

        // OpenGL Draw commands
        drawFloor();
        drawCannon();
        drawTarget1();
        drawTarget2();
        drawObs1();
        drawObs3();
        drawObs2_1();
        drawObs2_2();
        CheckFloorCollisions(x_cannonball, y_cannonball,-6.0f,6.0f,-3.0f); //Floor
						CheckFloorCollisions(x_cannonball, y_cannonball,-1.2f,-0.8f,-2.0f); //Obs1 ka top
						CheckFloorCollisions(x_cannonball, y_cannonball,-2.2f,-1.8f,-2.25f); //Obs2_1
						CheckFloorCollisions(x_cannonball, y_cannonball,0.8f,1.2f,-2.25f); //Obs2_2
						CheckFloorCollisions(x_cannonball, y_cannonball,-0.2f,0.2f,-1.5f); //Obs3
						if(checkCollisionObs(x_cannonball, y_cannonball,-1.2f,-0.8f,-3.0f,-2.0f,t)) //Obs1
						{
							///cout << "ping!!!\n";
							///cout << "IN IF vx before: "<< vx << "\n";
							vx = -1 * e * vx;
							///cout << "e: "<< e << "\n";
							///cout << "IN IF vx after: "<< vx << "\n";
						}
						if(checkCollisionObs(x_cannonball, y_cannonball,-2.2f,-1.8f,-3.0f,-2.25f,t)) //Obs2_1
						{
							///cout << "ping!!!\n";
							///cout << "IN IF vx before: "<< vx << "\n";
							vx = -1 * e * vx;
							///cout << "e: "<< e << "\n";
							///cout << "IN IF vx after: "<< vx << "\n";
						}
						if(checkCollisionObs(x_cannonball, y_cannonball,0.8f,1.2f,-3.0f,-2.25f,t)) //Obs2_2
						{
							///cout << "ping!!!\n";
							///cout << "IN IF vx before: "<< vx << "\n";
							vx = -1 * e * vx;
							///cout << "e: "<< e << "\n";
							///cout << "IN IF vx after: "<< vx << "\n";
						}
						if(checkCollisionObs(x_cannonball, y_cannonball,-0.2f,0.2f,-3.0f,-1.5f, t)) //Obs3
						{
							///cout << "ping!!!\n";
							///cout << "IN IF vx before: "<< vx << "\n";
							vx = -1 * e * vx;
							///cout << "e: "<< e << "\n";
							///cout << "IN IF vx after: "<< vx << "\n";
						}
		/*else
		{
			vx = ux;
		}*/
        if((fire == 1 || (in_air_flag == 1 && collision_flag == 0)) && (x_cannonball > -4.5f && x_cannonball < 4.5f))
		{	
			///cout<<"if\n";
			///cout << "x_cannonball: "<< x_cannonball << "\n";
			///cout << "thita ball (if):  " << thita_ball << " thita: " << thita << "\n";
			
			//cout << "if\n";
			//cout << "fire flag value - "<<fire <<"\n";
			//cout << "in_air_flag - " << in_air_flag << "\n";
			//cout << "collision_flag - " << collision_flag << "\n";
			drawCannonBall(x_cannonball,y_cannonball);
  	
    		//if(y_cannonball > -2.85f)
    		t+=0.01;
		}
		else if(collision_flag == 1 && in_air_flag == 0)
		{
			///cout << "thita ball elif"<< thita_ball << " thita: " << thita<<"\n";
			///cout << "obs_collision : " <<obs_collision << "\n";
			//cout << "fire flag value - "<<fire <<"\n";
			//cout << "in_air_flag - " << in_air_flag << "\n";
			//cout << "collision_flag - " << collision_flag << "\n";
	
			in_air_flag = 1;
			collision_flag = 0;
			//x_till_collision += ((float)(v*v*sin(2*(float)((thita_ball+90)*M_PI/180.0f)) / g) + 0.1f); 
			//x_till_collision *= 1.2f;
			
			//if(obs_collision == 1)
			//{
			//    thita_ball = (((float)(M_PI/2) + beta - delta) * (float)(180.0f / M_PI)) + 90;
			//}
			
			if(obs_collision == 0)
			{
			
				u *= e;
				t = 0;	
				y_cannonball = y_till_collision;
			}
			else
			{
				///cout << "ping3333!!!!!! \n";
				t+=0.01;
			}
			///cout << "x_till_collision :" << x_till_collision << "\n";
			///cout << " v - " << v << "\n";
			///cout << "t - " << t << "\n";
			///cout << "thita_ball : " << thita_ball << "\n";
		}

		if ((x_cannonball < -4.5f || x_cannonball > 4.5f))
		{
			///cout << "Thita ball (It Should stop now!!!!!!!!!!)"<< thita_ball << " thita: " << thita<<"\n" ;
			in_air_flag = 0;
			collision_flag = 0;
			t = 0;
			vx = ux;
			x_till_collision = -3.0f;
			y_till_collision = -2.75f;
			fire = 0;
			u = 4.0f;
			last_update_time = glfwGetTime();
		}
		
		///cout << "y_cannonball in int main:" << y_cannonball << "\n";
		///cout << "thita ball after LOOP" << thita_ball << " thita: " << thita << "\n";
        //drawObs1();
        //drawTargets();
        // Swap Frame Buffer in double buffering
        glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
        glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 10)
        {  // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
            in_air_flag = 0;
			collision_flag = 0;
			t = 0;
			vx = ux;
			x_till_collision = -3.0f;
			y_till_collision = -2.75f;
			fire = 0;
			u = 4.0f;
            last_update_time = current_time;
        }
    }

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
