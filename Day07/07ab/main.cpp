#include <charconv>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string>
#include <variant>
#include <unordered_map>

class File {
public:
	File() noexcept = default;

	File(std::string const& name, long long size) noexcept : Name(name), Size(size) {}

	std::string Name{};
	long long Size{};
};

class Directory;

using Entry = std::variant<File, Directory>;

class Directory {
public:
	Directory() noexcept = default;

	Directory(std::string const& name, Directory* parent = nullptr) noexcept : Name(name), Path((parent ? parent->Path : "") + "/" + name), Parent(parent) {}

	void AddEntry(File&& entry) {
		auto entryName = entry.Name;

		Entries.emplace(std::move(entryName), std::move(entry));
	}

	void AddEntry(Directory&& entry) {
		auto entryName = entry.Name;

		Entries.emplace(std::move(entryName), std::move(entry));
	}

	Entry& GetEntry(std::string const& name) {
		return Entries[name];
	}

	Directory* GetParentDirectory() const {
		return Parent;
	}

	std::unordered_map<std::string, Entry>& GetEntries() {
		return Entries;
	}

	std::string const& GetName() const {
		return Name;
	}

	std::string const& GetPath() const {
		return Path;
	}

private:
	std::string Name{}, Path{};
	std::unordered_map<std::string, Entry> Entries{};
	Directory* Parent{};
};

long long strToInt(std::string const& str) {
	long long result{};

	[[likely]]
	if(std::from_chars(str.data(), str.data() + str.length(), result).ec == std::errc{}) {
		return result;
	}

	std::cerr << "Could not convert string " << str << " to int.\n";
	exit(3);
}


int main() {

	Directory root("/");

	std::fstream input("input.txt");

	std::string line{};

	Directory* currentDirectory = &root;

	auto performCd = [&currentDirectory, &root](std::string const& targetDirectory) mutable -> void {
		if(targetDirectory == "/") {
			currentDirectory = &root;
		}
		else if(targetDirectory == "..") {
			auto parentDirectory = currentDirectory->GetParentDirectory();

			[[unlikely]]
			if(!parentDirectory) {
				std::cerr << "Parent directory does not exist!\n";
				exit(2);
			}

			currentDirectory = parentDirectory;
		}
		else {
			auto& entry = currentDirectory->GetEntry(targetDirectory);

			auto& directory = [&entry]() -> auto& {
				try {
					return std::get<Directory>(entry);
				}
				catch(std::bad_variant_access const& ex) {
					std::cerr << "Entry is not a directory\n" << ex.what() << std::endl;
					exit(3);
				}
			}();

			currentDirectory = &directory;
		}
	};

	auto performLs = []() -> void {};

	struct GetDirectorySizes {

		GetDirectorySizes(std::unordered_map<std::string, long long>& output) : Output(output) {};

		long long operator()(Directory& directory) const noexcept {
			
			long long size = 0;

			for(auto& [_, entry] : directory.GetEntries()) {
				
				size += std::visit(GetDirectorySizes(Output), entry);

			}

			Output[directory.GetPath()] = size;

			return size;

		}

		long long operator()(File& file) const noexcept {
			
			return file.Size;

		}

		std::unordered_map<std::string, long long>& Output;
	};

	auto addLsEntry = [&currentDirectory](std::string arg0, std::string arg1) mutable -> void {
		if(arg0 == "dir") {
			currentDirectory->AddEntry(Directory(arg1, currentDirectory));
		}
		else {
			long long const size = strToInt(arg0);

			currentDirectory->AddEntry(File(arg1, size));
		}
	};

	while(std::getline(input, line)) {

		std::cout << line << '\n';
		
		auto split = std::views::split(line, ' ');

		int const argCount = int(std::distance(split.begin(), split.end()));

		[[unlikely]]
		if(argCount < 2) {
			std::cerr << "Not enough arguments! (" << argCount << ")\n";
			exit(1);
		}

		auto arg0it = *split.begin();
		auto arg1it = *std::next(split.begin());

		std::string arg0(arg0it.begin(), arg0it.end());
		std::string arg1(arg1it.begin(), arg1it.end());

		if(arg0 == "$") {
			
			if(arg1 == "cd") {
				
				[[unlikely]]
				if(argCount < 3) {
					std::cerr << "Not enough arguments! (" << argCount << ")\n";
					exit(1);
				}

				
				auto arg2it = *std::next(split.begin(), 2);

				std::string arg2(arg2it.begin(), arg2it.end());

				performCd(arg2);

			}

			else if(arg1 == "ls") {
				
				performLs();

			}

		}

		else {
				
			addLsEntry(arg0, arg1);

		}

	}

	std::unordered_map<std::string, long long> directorySizes{};

	Entry rootEntry(root);
	std::visit(GetDirectorySizes(directorySizes), rootEntry);

	long long satisfactoryDirectoriesSize = 0;
	long long totalSpace = 70000000L;
	long long requiredSpace = 30000000L;
	long long usedSpace = directorySizes[root.GetPath()];
	long long freeSpace = totalSpace - usedSpace;

	long long minSpaceToFree = requiredSpace - freeSpace;

	long long smallestSufficientFolderSize = usedSpace;

	std::cout << '\n';

	for(auto const& [directoryPath, directorySize] : directorySizes) {

		std::cout << '[' << directoryPath << ',' << directorySize << "]\n";
		
		if(directorySize <= 100000) {
			satisfactoryDirectoriesSize += directorySize;
		}

		if(directorySize >= minSpaceToFree && directorySize < smallestSufficientFolderSize) {
			smallestSufficientFolderSize = directorySize;
		}

	}

	std::cout << "Size of satiscaftory directories: " << satisfactoryDirectoriesSize << '\n';
	std::cout << "Smallest sufficient folder to delete: " << smallestSufficientFolderSize << '\n';

	return 0;
}