#include "catch.hpp"

#include <dekaf2/kwebobjects.h>

using namespace dekaf2;

extern KString print_diff(KStringView s1, KStringView s2);

TEST_CASE("KWebObjects")
{

	SECTION("main")
	{
		static constexpr KStringView sSerialized = R"(<!DOCTYPE html>
<html lang="en">
	<head>
		<title>
			My First Web Page
		</title>
		<style>
			body {
font-family: Verdana, sans-serif; background-color: powderblue
}

.nodecoration {
text-decoration: none
}

		</style>
	</head>
	<body>
		<p>
			<img alt="this is a test image" class=".nodecoration" loading="lazy" src="http://some.image.url/at/a/path.png"/><br/>
			<img alt="this is a test image" class=".nodecoration" loading="lazy" src="http://some.image.url/at/a/path.png"/>
		</p>
		<div class=".nodecoration" id="FormDiv">
			<p>
				<form accept-charset="utf-8" action="MyForm">
					<button name="q" type="submit" value="button1">
						Schaltfläche 1
					</button>
					<button name="q" type="submit" value="button2">
						Schaltfläche 2
					</button>
					<button formenctype="multipart/form-data" formmethod="post" name="q" type="submit" value="button3">
						Schaltfläche 3
					</button>
				</form>
			</p>
			<p class=".nodecoration" id="TextPar">
				hello world<a href="/images/test.png"><img alt="preview" id="IMGID" src="/Images/test_small.png"/></a>
			</p>
			<form accept-charset="utf-8" action="/Setup/">
				<fieldset>
					<legend>
						System Setup
					</legend>
					<label>
						<input name="datadir" size="60" type="text" value="abcdef"/>
						shared data directory
					</label>
					<br/>
					<label>
						<input name="outputdir" size="60" type="text"/>
						shared data directory
					</label>
					<div hidden></div>
					<div hidden></div>
				</fieldset>
				<fieldset>
					<legend>
						Email
					</legend>
					<label>
						<input name="_fobj1" size="30" type="text"/>
						target email
					</label>
					<br/>
					<label>
						<input name="mailfrom" size="30" type="text"/>
						sender email
					</label>
					<br/>
					<label>
						<input name="_fobj2" size="30" type="text"/>
						smtp relay
					</label>
				</fieldset>
				<fieldset>
					<legend>
						Display
					</legend>
					<label>
						<input max="0.99" min="0.01" name="confidence" step="0.025" type="number" value="0.55"/>
						confidence value, from 0.0 .. 1.0
					</label>
					<br/>
					<label>
						<input name="nodetect" type="checkbox"/>
						no object detection
					</label>
					<br/>
					<label>
						<input checked name="nodisplay" type="checkbox"/>
						do not open camera windows
					</label>
					<br/>
					<label>
						<input name="nooverlay" type="checkbox"/>
						no detection overlay
					</label>
					<br/>
					<label>
						<input name="motion" type="checkbox"/>
						show motion
					</label>
					<label>
						<input max="1000" min="-1" name="_fobj3" step="1" type="number" value="1000"/>
						motion area
					</label>
					<br/>
					<label>
						<input max="86400" min="1" name="interval" type="number" value="60"/>
						seconds between alarms
					</label>
					<label>
						<select name="selection" size="1">
							<option value="_fobj4">
								Sel1
							</option>
							<option value="_fobj5">
								Sel2
							</option>
							<option value="_fobj6">
								Sel3
							</option>
							<option selected value="_fobj7">
								Sel4
							</option>
						</select>
						please choose a value
					</label>
				</fieldset>
				<fieldset>
					<legend>
						Radios
					</legend>
					<label>
						<input name="_fobj8" type="radio" value="red"/>
						red
					</label>
					<label>
						<input name="_fobj8" type="radio" value="green"/>
						green
					</label>
					<label>
						<input name="_fobj8" type="radio" value="blue"/>
						blue
					</label>
					<label>
						<input name="_fobj8" type="radio" value="yellow"/>
						yellow
					</label>
				</fieldset>
			</form>
		</div>
		<a href="link" title="title"><img alt="AltText" loading="lazy" src="another/link"/></a>
		<a href="link" title="title"><img alt="AltText" loading="lazy" src="another/link"/></a>
	</body>
</html>
)";
		struct Config
		{
			KString sDataDir { "this/is/preset" };
			KString sOutputDir;
			KString sMailTo;
			KString sMailFrom;
			KString sSMTPServer;
			KString sRadio;
			KString sSelection { "Sel2" };
			int     iMotionArea = 0;
			std::chrono::high_resolution_clock::duration dInterval { 0 };
			double m_fMinConfidence { 0.0 };
			bool bNoDetect   { false };
			bool bNoDisplay  { false };
			bool bNoOverlay  { false };
			bool bShowMotion { false };
		};

		Config m_Config;

		url::KQueryParms Parms;
		Parms.Add("datadir"   , "abcdef");
		Parms.Add("mailto"    , "xyz"   );
		Parms.Add("nodisplay" , "on"    );
		Parms.Add("interval"  , "60"    );
		Parms.Add("confidence", "0.55"  );
		Parms.Add("_fobj3"    , "1001"  ); // this value is too large for the object (max = 1000)
		Parms.Add("selection" , "_fobj7");

		html::Page page("My First Web Page", "en");

		html::Class Class("body", "font-family: Verdana, sans-serif; background-color: powderblue");
		page.AddClass(Class);

		html::Class NoDecoration(".nodecoration", "text-decoration: none");
		page.AddClass(NoDecoration);

		html::Image image("http://some.image.url/at/a/path.png", "Image1", "", NoDecoration);
		image.SetDescription("this is a test image");
		image.SetLoading(html::Image::LAZY);

		auto& body = page.Body();
		{
			auto& par = body.Add(html::Paragraph());
			par.Add(image);
			par.Add(html::Break());
			par += image;
		}

		auto& div = body.Add(html::Div("FormDiv", NoDecoration));

		{
			auto& par = div.Add(html::Paragraph());
			auto& form = par.Add(html::Form("MyForm"));
			form += html::Button("Schaltfläche 1").SetName("q").SetValue("button1");
			form += html::Button("Schaltfläche 2").SetName("q").SetValue("button2");
			form += html::Button("Schaltfläche 3").SetName("q").SetValue("button3").SetFormMethod(html::Button::POST).SetFormEncType(html::Button::FORMDATA);
		}
		
		{
			auto& par = div.Add(html::Paragraph("TextPar", NoDecoration));
			par.AddText("hello world");
			par += html::Link("/images/test.png").Append(html::Image("/Images/test_small.png", "preview").SetID("IMGID"));
		}

		{
			auto& form = div.Add(html::Form("/Setup/"));

			kDebug(2, "start");
			{
				auto& group = form.Add(html::FieldSet("System Setup"));
				group += html::TextInput(m_Config.sDataDir   , "datadir"  ).SetLabelAfter("shared data directory").SetSize(60);
				group += html::Break();
				html::TextInput ti(m_Config.sOutputDir   , "outputdir"  );
				ti.SetLabelAfter("shared data directory");
				group += ti.SetSize(60);
				CHECK ( ti.Serialize().size() == 93 );
				group += html::Div().SetHidden(true);
				html::Div div;
				group += div.SetHidden(true);
				CHECK ( div.Serialize().size() == 20 );
			}

			{
				auto& group = form.Add(html::FieldSet("Email"));
				group += html::TextInput(m_Config.sMailTo    ).SetLabelAfter("target email").SetSize(30);
				group += html::Break();
				group += html::TextInput(m_Config.sMailFrom  , "mailfrom"  ).SetLabelAfter("sender email").SetSize(30);
				group += html::Break();
				group += html::TextInput(m_Config.sSMTPServer).SetLabelAfter("smtp relay").SetSize(30);
			}

			{
				auto& group = form.Add(html::FieldSet("Display"));
				group += html::NumericInput(m_Config.m_fMinConfidence, "confidence").SetLabelAfter("confidence value, from 0.0 .. 1.0").SetRange(0.01, 0.99).SetStep(0.0250);
				group += html::Break();
				group += html::CheckBox(m_Config.bNoDetect   , "nodetect"  ).SetLabelAfter("no object detection"       );
				group += html::Break();
				group += html::CheckBox(m_Config.bNoDisplay  , "nodisplay" ).SetLabelAfter("do not open camera windows");
				group += html::Break();
				group += html::CheckBox(m_Config.bNoOverlay  , "nooverlay" ).SetLabelAfter("no detection overlay"      );
				group += html::Break();
				group += html::CheckBox(m_Config.bShowMotion , "motion"    ).SetLabelAfter("show motion"               );
				group += html::NumericInput(m_Config.iMotionArea).SetLabelAfter("motion area").SetMin(-1).SetMax(1000).SetStep(1);
				group += html::Break();
				group += html::DurationInput<std::chrono::seconds>(m_Config.dInterval, "interval").SetLabelAfter("seconds between alarms").SetRange(1, 86400);
				group += html::Selection(m_Config.sSelection).SetName("selection").SetOptions("Sel1,Sel2,Sel3,Sel4").SetLabelAfter("please choose a value");
			}

			{
				auto& group = form.Add(html::FieldSet("Radios"));
				group += html::RadioButton(m_Config.sRadio).SetValue("red"   ).SetLabelAfter("red"   );
				group += html::RadioButton(m_Config.sRadio).SetValue("green" ).SetLabelAfter("green" );
				group += html::RadioButton(m_Config.sRadio).SetValue("blue"  ).SetLabelAfter("blue"  );
				group += html::RadioButton(m_Config.sRadio).SetValue("yellow").SetLabelAfter("yellow");
			}

			form.Generate();
			form.Synchronize(Parms);

			body += html::Link("link").SetTitle("title").Append(html::Image("another/link", "AltText").SetLoading(html::Image::LAZY));
			body += html::LineBreak();
			body += html::Link("link").SetTitle("title").Append(html::Image("another/link", "AltText").SetLoading(html::Image::LAZY));
			body += html::LineBreak();

			html::TextInput ti(m_Config.sMailTo, "mailto");
			ti.Synchronize(Parms);
		}

		KString sCRLF = sSerialized;
		sCRLF.Replace("\n", "\r\n");

		CHECK( page.Print() == sCRLF );

		auto sDiff = print_diff(page.Print(), sCRLF);
		if (!sDiff.empty())
		{
			FAIL_CHECK ( sDiff );
		}

	}
}
