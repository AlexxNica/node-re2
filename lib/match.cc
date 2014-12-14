#include "./wrapped_re2.h"

#include <memory>
#include <vector>

#include <node_buffer.h>


using std::vector;

using v8::Array;
using v8::Integer;
using v8::Local;
using v8::String;

using node::Buffer;


NAN_METHOD(WrappedRE2::Match) {
	NanScope();

	// unpack arguments

	WrappedRE2* re2 = ObjectWrap::Unwrap<WrappedRE2>(args.This());
	if (!re2) {
		NanReturnNull();
	}

	vector<char> buffer;

	char*  data;
	size_t size;
	bool   isBuffer = false;

	if (Buffer::HasInstance(args[0])) {
		isBuffer = true;
		size = Buffer::Length(args[0]);
		data = Buffer::Data(args[0]);
	} else {
		Local<String> t(args[0]->ToString());
		buffer.resize(t->Utf8Length() + 1);
		t->WriteUtf8(&buffer[0]);
		size = buffer.size() - 1;
		data = &buffer[0];
	}

	vector<StringPiece> groups;
	StringPiece str(data, size);

	// actual work

	if (re2->global) {
		// global: collect all matches

		StringPiece match;
		size_t lastIndex = 0;

		while (re2->regexp.Match(str, lastIndex, size, RE2::UNANCHORED, &match, 1)) {
			groups.push_back(match);
			lastIndex = match.data() - data + match.size();
		}

		if (groups.empty()) {
			NanReturnNull();
		}
	} else {
		// non-global: just like exec()

		groups.resize(re2->regexp.NumberOfCapturingGroups() + 1);
		if (!re2->regexp.Match(str, 0, size, RE2::UNANCHORED, &groups[0], groups.size())) {
			NanReturnNull();
		}
	}

	// form a result

	Local<Array> result = NanNew<Array>();

	if (isBuffer) {
		for (size_t i = 0, n = groups.size(); i < n; ++i) {
			const StringPiece& item = groups[i];
			result->Set(i, NanNewBufferHandle(item.data(), item.size()));
		}
		if (!re2->global) {
			result->Set(NanNew("index"), NanNew<Integer>(groups[0].data() - data));
			result->Set(NanNew("input"), args[0]);
		}
	} else {
		for (size_t i = 0, n = groups.size(); i < n; ++i) {
			const StringPiece& item = groups[i];
			result->Set(i, NanNew<String>(item.data(), item.size()));
		}
		if (!re2->global) {
			result->Set(NanNew("index"), NanNew<Integer>(getUtf16Length(data, groups[0].data())));
			result->Set(NanNew("input"), args[0]);
		}
	}

	NanReturnValue(result);
}
