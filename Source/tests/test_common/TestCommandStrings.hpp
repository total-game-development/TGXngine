#pragma once

#include <gtest/gtest.h>
#include "StringUtils.hpp"

// The engine passes what a player asked for between a module and the executable
// as "key:value,key:value". A module writes one and the executable reads it back,
// so the two have to agree on what a field is.
TEST(CommandStrings, ReadsAFieldOutOfTheMiddle)
{
	EXPECT_EQ(StringField("team:technology,cost:500,key:tank", "cost"), "500");
}

TEST(CommandStrings, ReadsTheFirstAndLastField)
{
	const std::string command = "team:technology,cost:500,key:tank";

	EXPECT_EQ(StringField(command, "team"), "technology");
	EXPECT_EQ(StringField(command, "key"), "tank");
}

TEST(CommandStrings, ReadsNothingForAFieldThatIsNotThere)
{
	EXPECT_EQ(StringField("team:technology", "cost"), "");
}

TEST(CommandStrings, DoesNotMatchAFieldByItsTail)
{
	// "cost" must not be found inside "powercost".
	EXPECT_EQ(StringField("powercost:9", "cost"), "");
}

TEST(CommandStrings, KeepsAValueThatHasItsOwnColons)
{
	EXPECT_EQ(StringField("at:12:30,name:x", "at"), "12:30");
}

TEST(CommandStrings, ReadsAnEmptyValue)
{
	EXPECT_EQ(StringField("team:,cost:1", "team"), "");
	EXPECT_EQ(StringField("team:,cost:1", "cost"), "1");
}

TEST(CommandStrings, SurvivesAStringWithNoFieldsAtAll)
{
	EXPECT_EQ(StringField("", "cost"), "");
	EXPECT_EQ(StringField("nonsense", "cost"), "");
}

// What Renderer::Purchase appends and Sidebar::Settle reads back.
TEST(CommandStrings, ReadsTheAnswerAppendedToARequest)
{
	const std::string settled = "team:technology,cost:500,key:tank,paid:true";

	EXPECT_EQ(StringField(settled, "paid"), "true");
	EXPECT_EQ(StringField(settled, "key"), "tank");
}
