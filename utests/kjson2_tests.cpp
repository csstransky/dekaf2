#include "catch.hpp"
#include <dekaf2/kjson2.h>
#include <dekaf2/krow.h>
#include <vector>

#ifndef DEKAF2_IS_WINDOWS

using namespace dekaf2;

namespace {
KJSON2 jsonAsPar(const KJSON2& json = KJSON2{})
{
	return json;
}
}

TEST_CASE("KJSON2")
{
	SECTION("Basic construction")
	{
		KJSON2 j1;
		j1.Parse(R"(
				   {
				       "key1": "val1",
				       "key2": "val2",
		               "list": [1, 0, 2],
				       "object": {
		                   "currency": "USD",
		                   "value": 42.99
				       }
				   }
				   )");

		KString value;
		value = kjson::Select(j1, "key1");
		CHECK ( value == "val1" );
		value = j1.Select("key1").String();
		CHECK ( value == "val1" );
		value = j1.Select("key1");
		CHECK ( value == "val1" );
		value = j1.Select("key3").String();
		CHECK ( value == "" );
		value = j1.Select("key3");
		CHECK ( value == "" );
		value = j1["key1"];
		CHECK ( value == "val1" );
		value = j1["key2"_ksz];
		CHECK ( value == "val2" );

		value = j1["object"]["currency"];
		CHECK ( value == "USD" );
		value = j1["object.currency"];
		CHECK ( value == "USD" );
		double d = j1["object"]["value"];
		CHECK ( d == 42.99 );
		KJSON2 j2 = j1["object"_ksv];
		value = j2["currency"];
		CHECK ( value == "USD" );
		d = j2["value"];
		CHECK ( d == 42.99 );
		KString sKey;
		sKey = "key1";
		value = j1[sKey];
		CHECK ( value == "val1" );

		if (!kjson::IsObject(j1, "object"))
		{
			CHECK ( false );
		}
		if (!kjson::IsObject(j1, "object"_ks))
		{
			CHECK ( false );
		}
		if (!kjson::IsObject(j1, "object"_ksv))
		{
			CHECK ( false );
		}
		if (!kjson::IsObject(j1, "object"_ksz))
		{
			CHECK ( false );
		}
		if (j1.is_object())
		{
			if (j1["object"].is_object())
			{
				auto obj = j1["object"];
				if (!obj.is_object())
				{
					CHECK ( false );
				}
				value = obj["currency"];
				CHECK ( value == "USD" );
			}
		}
		else
		{
			CHECK ( false );
		}

		value = kjson::GetString(j1, "key2");
		CHECK ( value == "val2" );

		value = kjson::GetString(j1, "not existing");
		CHECK ( value == "" );
	}

	SECTION("Initializer list construction")
	{
		KJSON2 j1 = {
		    {"pi", 3.141},
		    {"happy", true},
		    {"key1", "val1"},
		    {"key2", "val2"},
		    {"nothing", nullptr},
		    {"answer", {
		         {"everything", 42}
		     }},
		    {"list", {1, 0, 2}},
		    {"object", {
		         {"currency", "USD"},
		         {"value", 42.99}
		     }}
		};

		KString value = j1["key1"];
		CHECK ( value == "val1" );
		value = j1["key2"];
		CHECK ( value == "val2" );

		value = j1["object"]["currency"];
		CHECK ( value == "USD" );
		double d = j1["object"]["value"];
		CHECK ( d == 42.99 );
		KJSON j2 = j1["object"];
		value = j2["currency"];
		CHECK ( value == "USD" );
		d = j2["value"];
		CHECK ( d == 42.99 );
	}

	SECTION("LJSON basic ops")
	{
		KJSON2 obj;
		obj["one"] = 1;
		obj["two"] = 2;
		KJSON child;
		child["duck"] = "donald";
		child["pig"]  = "porky";
		KJSON arr1 = KJSON::array();
		KJSON arr2 = KJSON::array();
		arr1 += child;
		obj["three"] = arr1;
		obj["four"] = arr2;
		obj["five"] = "string";
		CHECK ( obj["five"].String() == "string" );
	}

	SECTION("KJSON - KROW interoperability")
	{
		KROW row;
		row.AddCol("first", "value1");
		row.AddCol("second", "value2");
		row.AddCol("third", 12345);

		CHECK( row["first"] == "value1" );
		CHECK( row["second"] == "value2" );
		CHECK( row["third"].Int64() == 12345 );

		KJSON obj = row;
		CHECK( obj["first"] == "value1" );
		CHECK( obj["second"] == "value2" );
		CHECK( obj["third"] == 12345 );
	}

	SECTION("KROW - KJSON interoperability")
	{
		KJSON json;
		json["first"] = "value1";
		json["second"] = "value2";
		json["third"] = 12345;

		KROW row = json;
		CHECK( row["first"] == "value1" );
		CHECK( row["second"] == "value2" );
		CHECK( row["third"].Int64() == 12345 );

		KJSON json2 = row;

		CHECK ( json == json2 );

		KROW row2;
		row2.AddCol("fourth", "value4");

		row2 += json;
		CHECK( row2["first"] == "value1" );
		CHECK( row2["second"] == "value2" );
		CHECK( row2["third"].Int64() == 12345 );
		CHECK( row2["fourth"] == "value4" );
	}

	SECTION("Print")
	{
		KJSON json = {
			{"pi", 3.141529},
			{"happy", true},
			{"key1", "val1"},
			{"key2", "val2"},
			{"days", 365 },
			{"nothing", nullptr},
			{"answer", {
				{"everything", 42}
			}},
			{"list", {1, 0, 2}},
			{"object", {
				{"currency", "USD"},
				{"value", 42.99}
			}}
		};

		KString sString = Print(json);
		static constexpr KStringView sExpected1 = R"({"answer":{"everything":42},"days":365,"happy":true,"key1":"val1","key2":"val2","list":[1,0,2],"nothing":null,"object":{"currency":"USD","value":42.99},"pi":3.141529})";
		CHECK ( sString == sExpected1 );

		sString = Print(json["key1"]);
		static constexpr KStringView sExpected2 = "val1";
		CHECK ( sString == sExpected2 );

		sString = Print(json["days"]);
		static constexpr KStringView sExpected3 = "365";
		CHECK ( sString == sExpected3 );

		sString = Print(json["pi"]);
		static constexpr KStringView sExpected4 = "3.141529";
		CHECK ( sString == sExpected4 );

		sString = Print(json["wrong"]);
		static constexpr KStringView sExpected5 = "NULL";
		CHECK ( sString == sExpected5 );

		sString = Print(json["nothing"]);
		static constexpr KStringView sExpected6 = "NULL";
		CHECK ( sString == sExpected6 );

		sString = Print(json["list"]);
		static constexpr KStringView sExpected7 = "[1,0,2]";
		CHECK ( sString == sExpected7 );

		sString = Print(json["answer"]);
		static constexpr KStringView sExpected8 = "{\"everything\":42}";
		CHECK ( sString == sExpected8 );

	}

	SECTION("Contains (array)")
	{
		KJSON json;
		json = KJSON::array();
		json += "value1";
		json += "value2";
		json += "value3";
		json += "value4";
		json += "value6";
		json += "value7";

		CHECK ( Contains(json, "value3") == true  );
		CHECK ( Contains(json, "value9") == false );
	}

	SECTION("Contains (object)")
	{
		KJSON json;
		json = KJSON::object();
		json += { "key1", "value1" };
		json += { "key2", "value2" };
		json += { "key3", "value3" };
		json += { "key4", "value4" };
		json += { "key5", "value5" };
		json += { "key6", "value6" };
		json += { "key7", "value7" };

		CHECK ( Contains(json, "key3") == true  );
		CHECK ( Contains(json, "key9") == false );
	}

	SECTION("GetObjectRef")
	{
		KJSON2 json = {
			{"pi", 3.141529},
			{"happy", true},
			{"key1", "val1"},
			{"key2", "val2"},
			{"days", 365 },
			{"nothing", nullptr},
			{"answer", {
				{"everything", 42}
			}},
			{"list", {1, 0, 2}},
			{"object", {
				{"currency", "USD"},
				{"value", 42.99}
			}}
		};

		const KJSON& object = kjson::GetObjectRef(json, "object");

		CHECK ( object["currency"] == "USD" );
		CHECK ( object["value"]    == 42.99 );
	}

	SECTION("implicit instantiation")
	{
		KString sVal1 { "val1" };
		auto json = jsonAsPar(
		{
			{"pi", 3.141529},
			{"happy", true},
			{"key1", sVal1},
			{"key2", "val2"},
			{"days", 365 }
		});

		CHECK ( json.dump() == R"({"days":365,"happy":true,"key1":"val1","key2":"val2","pi":3.141529})" );

		json = jsonAsPar(
		{{
			{"pi", 3.141529},
			{"happy", true},
			{"key1", sVal1},
			{"key2", "val2"},
			{"days", 365 }
		},
		{
			{"pa", 3.141529},
			{"happy", false},
			{"key1", "val3"},
			{"key2", "val4"},
			{"days", 180 }
		}});

		CHECK ( json.dump() == R"([{"days":365,"happy":true,"key1":"val1","key2":"val2","pi":3.141529},{"days":180,"happy":false,"key1":"val3","key2":"val4","pa":3.141529}])" );
	}

	SECTION("null")
	{
		KJSON json;
		CHECK ( json.is_null() );
		CHECK ( json.dump(-1) == "null" );
		json = KJSON::object();
		CHECK ( json.is_object() );
		CHECK ( json.dump(-1) == "{}" );
		json = KJSON::parse("null");
		CHECK ( json.is_null() );
		CHECK ( json.dump(-1) == "null" );
	}

	SECTION("KStringView")
	{
		std::vector<KStringView> View { "one", "two", "three", "four", "five" };
		KJSON json
		{
			{ "view", View   }
		};
		CHECK ( json.dump(-1) == R"({"view":["one","two","three","four","five"]})" );
	}

	SECTION("Select")
	{
		KJSON2 j1 = {
			{"pi", 3.141},
			{"happy", true},
			{"key1", "val1"},
			{"key2", "val2"},
			{"nothing", nullptr},
			{"answer", {
				{"everything", 42},
				{"nothing", "naught"},
				{"few", { "one", "two", "three"}}
			}},
			{"ilist", {1, 0, 2}},
			{"slist", {"one", "two", "three"}},
			{"object", {
				{"currency", "USD"},
				{"value", 42.99}
			}}
		};

		{
			const KString& sString = j1.Select("/key2");
			CHECK ( sString == "val2" );
		}
		{
			const KString& sString = j1.Select("key2");
			CHECK ( sString == "val2" );
		}
		{
			const KString& sString = j1.Select("/answer/nothing");
			CHECK ( sString == "val2" );
		}
		{
			const KString& sString = j1.Select("answer.nothing");
			CHECK ( sString == "val2" );
		}
		{
			const KString& sString = j1.Select("/answer/few/1");
			CHECK ( sString == "two" );
		}
		{
			const KString& sString = j1.Select("answer.few[0]");
			CHECK ( sString == "one" );
		}
		{
			const KString& sString = j1.Select("/slist/0");
			CHECK ( sString == "one" );
		}
		{
			const KString& sString = j1.Select("/slist/1");
			CHECK ( sString == "two" );
		}
		{
			const KString& sString = j1.Select("slist[0]");
			CHECK ( sString == "one" );
		}
		{
			const KString& sString = j1.Select("slist[0]");
			CHECK ( sString == "two" );
		}
		{
			const KString& sString = j1.Select("slist[4]");
			CHECK ( sString == "" );
		}
		{
			const KString& sString = j1.Select("answer/unknown");
			CHECK ( sString == "" );
		}
	}

	SECTION("SelectString")
	{
		KJSON2 j1 = {
			{"pi", 3.141},
			{"happy", true},
			{"key1", "val1"},
			{"key2", "val2"},
			{"nothing", nullptr},
			{"answer", {
				{"everything", 42},
				{"nothing", "naught"},
				{"few", { "one", "two", "three"}}
			}},
			{"ilist", {1, 0, 2}},
			{"slist", {"one", "two", "three"}},
			{"object", {
				{"currency", "USD"},
				{"value", 42.99}
			}}
		};

		CHECK ( j1.Select("/key2").String() == "val2" );
		CHECK ( j1.Select("key2" ).String() == "val2" );
		CHECK ( j1.Select("/answer/nothing").String() == "naught" );
		CHECK ( j1.Select("answer.nothing" ).String() == "naught" );
		CHECK ( j1.Select("/answer/few/1"  ).String() == "two" );
		CHECK ( j1.Select("answer.few[0]"  ).String() == "one" );
		CHECK ( j1.Select("/slist/0").String() == "one" );
		CHECK ( j1.Select("/slist/1").String() == "two" );
		CHECK ( j1.Select("slist[0]").String() == "one" );
		CHECK ( j1.Select("slist[1]").String() == "two" );
		CHECK ( j1.Select("slist[4]").String() == ""    );
		CHECK ( j1.Select("/answer/unknown").String() == "" );
		CHECK ( j1.Select("/answer/unknown").String() != "something" );
	}

	SECTION("SelectObject")
	{
		KJSON j1 = {
			{"pi", 3.141},
			{"happy", true},
			{"key1", "val1"},
			{"key2", "val2"},
			{"nothing", nullptr},
			{"answer", {
				{"everything", 42},
				{"nothing", "naught"},
				{"few", { "one", "two", "three"}}
			}},
			{"ilist", {1, 0, 2}},
			{"slist", {"one", "two", "three"}},
			{"object", {
				{"currency", "USD"},
				{"value", 42.99}
			}}
		};

		CHECK ( kjson::SelectObject(j1, "/answer").dump() == R"({"everything":42,"few":["one","two","three"],"nothing":"naught"})" );
		CHECK ( kjson::SelectObject(j1, "/answer/nothing") == KJSON() );
		CHECK ( kjson::SelectObject(j1, "/pi") == KJSON() );
	}

}
#endif