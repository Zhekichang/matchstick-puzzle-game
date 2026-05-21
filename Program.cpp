#include "stdafx.h"
#include "MainForm.h"

using namespace System;
using namespace System::Windows::Forms;
using namespace MatchstickPuzzle;

[STAThreadAttribute]
int main(array<String^>^)
{
    Application::EnableVisualStyles();
    Application::SetCompatibleTextRenderingDefault(false);
    Application::Run(gcnew MainForm());
    return 0;
}
