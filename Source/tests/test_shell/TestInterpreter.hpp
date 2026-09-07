#pragma once

#include <gtest/gtest.h>
#include "Host.h"
#include "Interpreter.h"

namespace TGX::Shell
{
class RecordingHost : public Host
{
public:
	Vector<String> printed;
	Map<String, String> files;
	Vector<String> toggles;
	Vector<String> spawned;

	void Print(const String &message) override
	{
		printed.push_back(message);
	}

	bool ReadFile(const String &name, String &source) override
	{
		const auto found = files.find(name);

		if (found == files.end())
		{
			return false;
		}

		source = found->second;

		return true;
	}

	void WriteFile(const String &name, const String &source) override
	{
		files[name] = source;
	}

	void Toggle(const String &name, const String &value, bool active) override
	{
		toggles.push_back(name + "=" + value + (active ? ":on" : ":off"));
	}

	void Spawn(const String &command) override
	{
		spawned.push_back(command);
	}
};

inline Vector<String> RunSource(RecordingHost &host, const String &source, const Vector<String> &args = {})
{
	Interpreter interpreter(&host);
	interpreter.SetStepLimit(1000000ULL);

	Vector<String> errors;
	NodeRef program = interpreter.Produce(source, errors);

	if (!errors.empty())
	{
		return errors;
	}

	interpreter.Run(program, args);

	return host.printed;
}

TEST(ShellInterpreter, PrintsStringLiteral)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "print(\"hello\")");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "hello");
}

TEST(ShellInterpreter, EvaluatesArithmeticPrecedence)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "print(2 + 3 * 4)");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "14");
}

TEST(ShellInterpreter, EvaluatesParenthesisedExpression)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "print((2 + 3) * 4)");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "20");
}

TEST(ShellInterpreter, ConcatenatesStrings)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "print(\"a\" + \"b\")");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "ab");
}

TEST(ShellInterpreter, DeclaresAndReadsVariables)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "let x = 5 x = x + 2 print(x)");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "7");
}

TEST(ShellInterpreter, RejectsAssignmentToConstant)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "const x = 5 x = 6 print(x)");

	ASSERT_FALSE(output.empty());
	EXPECT_NE(output[0].find("constant"), String::npos);
}

TEST(ShellInterpreter, RunsIfElseBranches)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "if (1 < 2) { print(\"yes\") } else { print(\"no\") }");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "yes");
}

TEST(ShellInterpreter, RunsWhileLoop)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "let i = 0 while (i < 3) { print(i) i = i + 1 }");

	ASSERT_EQ(output.size(), 3u);
	EXPECT_EQ(output[0], "0");
	EXPECT_EQ(output[2], "2");
}

TEST(ShellInterpreter, RunsForLoop)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "for (let i = 0; i < 3; i = i + 1) { print(i) }");

	ASSERT_EQ(output.size(), 3u);
	EXPECT_EQ(output[0], "0");
	EXPECT_EQ(output[2], "2");
}

TEST(ShellInterpreter, BreaksOutOfLoop)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "let i = 0 while (i < 10) { if (i == 2) { break } print(i) i = i + 1 }");

	ASSERT_EQ(output.size(), 2u);
	EXPECT_EQ(output[1], "1");
}

TEST(ShellInterpreter, CallsFunctionWithReturnValue)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "function add(a, b) { return a + b } print(add(2, 3))");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "5");
}

TEST(ShellInterpreter, SupportsRecursion)
{
	RecordingHost host;
	const Vector<String> output =
		RunSource(host, "function fact(n) { if (n < 2) { return 1 } return n * fact(n - 1) } print(fact(5))");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "120");
}

TEST(ShellInterpreter, IndexesArrays)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "let a = [10, 20, 30] print(a[1])");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "20");
}

TEST(ShellInterpreter, IndexesArraysFromTheEnd)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "let a = [10, 20, 30] print(a[0 - 1])");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "30");
}

TEST(ShellInterpreter, ReadsObjectProperties)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "let o = { name: \"tgx\", version: 2 } print(o.name)");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "tgx");
}

TEST(ShellInterpreter, MeasuresLength)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "print(length(\"abcd\"))");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "4");
}

TEST(ShellInterpreter, SplitsStrings)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "let parts = 0 split(\"a,b,c\", \",\", parts) print(parts[1])");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "b");
}

TEST(ShellInterpreter, ReadsAndWritesFiles)
{
	RecordingHost host;
	host.files["notes"] = "payload";

	const Vector<String> output = RunSource(host, "print(read(\"notes\")) write(\"copy\", \"written\")");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "payload");
	EXPECT_EQ(host.files["copy"], "written");
}

TEST(ShellInterpreter, ExecRunsAnotherProgram)
{
	RecordingHost host;
	host.files["child"] = "print(\"from child\")";

	const Vector<String> output = RunSource(host, "exec(\"child\")");

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "from child");
}

TEST(ShellInterpreter, SpawnDelegatesToHost)
{
	RecordingHost host;
	RunSource(host, "spawn(\"worker one two\")");

	ASSERT_EQ(host.spawned.size(), 1u);
	EXPECT_EQ(host.spawned[0], "worker one two");
}

TEST(ShellInterpreter, ToggleDelegatesToHost)
{
	RecordingHost host;
	RunSource(host, "toggle(\"grid\", \"on\", 1)");

	ASSERT_EQ(host.toggles.size(), 1u);
	EXPECT_EQ(host.toggles[0], "grid=on:on");
}

TEST(ShellInterpreter, ReceivesCommandLineArguments)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "print(args[0])", {"first", "second"});

	ASSERT_EQ(output.size(), 1u);
	EXPECT_EQ(output[0], "first");
}

TEST(ShellInterpreter, IgnoresComments)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "// leading\nprint(1) /* inline */ print(2)");

	ASSERT_EQ(output.size(), 2u);
	EXPECT_EQ(output[0], "1");
	EXPECT_EQ(output[1], "2");
}

TEST(ShellInterpreter, StopsRunawayLoopsAtStepLimit)
{
	RecordingHost host;

	Interpreter interpreter(&host);
	interpreter.SetStepLimit(5000ULL);

	Vector<String> errors;
	NodeRef program = interpreter.Produce("let i = 0 while (1 == 1) { i = i + 1 }", errors);

	ASSERT_TRUE(errors.empty());

	interpreter.Run(program, {});

	ASSERT_FALSE(host.printed.empty());
	EXPECT_NE(host.printed.back().find("step budget"), String::npos);
}

TEST(ShellInterpreter, ReportsUnresolvedIdentifier)
{
	RecordingHost host;
	const Vector<String> output = RunSource(host, "print(missing)");

	ASSERT_FALSE(output.empty());
	EXPECT_NE(output[0].find("Cannot resolve"), String::npos);
}
} // namespace TGX::Shell
