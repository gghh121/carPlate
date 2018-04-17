#include "MyForm.h"

using namespace GHplaterecognition;

[STAThreadAttribute]

int main(array < System::String^ >^args) {
	Application::EnableVisualStyles();
	Application::Run(gcnew MyForm());
	return 0;
}
