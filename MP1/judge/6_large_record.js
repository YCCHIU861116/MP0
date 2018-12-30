// 測資六： 巨大 .loser_record
// 生成方式爲創建一個 a_or_b ，不斷將檔案改爲 a 再改爲 b
// 而每次改變後都會 commit ，一直 comit 到 .loser_record 檔案達到約 1GB

const child_process = require("child_process");
const fs = require("fs");
const constant = require("./constant.js");
const config = require("./config.js");

const directory = "6_large_record"

// 可透過修改此變數來減小檔案大小並加速
const RECORD_SIZE = config.LARGE_RECORD_SIZE;

function to_b(i) {
	return `# commit ${i}
[new_file]
[modified]
a_or_b
[copied]
(MD5)
a_or_b 92eb5ffee6ae2fec3ad71c777531578f
`;
}

function to_a(i) {
	return `# commit ${i}
[new_file]
[modified]
a_or_b
[copied]
(MD5)
a_or_b 0cc175b9c0f1b6a831c399e269772661
`;
}

let init_content = `# commit 1
[new_file]
a_or_b
[modified]
[copied]
(MD5)
a_or_b 0cc175b9c0f1b6a831c399e269772661
`;

const CHUNK_SIZE = 500;

// 爲加速，一次寫入 500 筆
function create_chunk(start) {
	let ret = "";
	for (let i = start; i < start + CHUNK_SIZE; i++) {
		if (i % 2 == 0) {
			ret += "\n";
			ret += to_b(i);
		} else {
			ret += "\n";
			ret += to_a(i);
		}
	}
	return [ret.length, ret];
}

let _commit_number = Math.floor(RECORD_SIZE / init_content.length);
const commit_number = _commit_number - (_commit_number % CHUNK_SIZE) + 1

function create_commit(path) {
	fs.writeFileSync(path, init_content);

	for (i = 1; i < commit_number; i += CHUNK_SIZE) {
		const [len, data] = create_chunk(i + 1);
		fs.appendFileSync(path, data);
	}
}

function create_log(number) {
	let ret = to_a(commit_number);
	for (let i = 1; i < number; i++) {
		ret += "\n";
		if ((commit_number - i) % 2 == 0) {
			ret += to_b(commit_number - i);
		} else {
			ret += to_a(commit_number - i);
		}
	}
	return ret;
}

module.exports = {
	description: "測資六： 巨大 .loser_record",
	directory: directory,
	create_data: function () {
		child_process.execSync(`mkdir -p data/${directory}`);
		fs.writeFileSync(`data/${directory}/a_or_b`, "a");
		create_commit(`data/${directory}/.loser_record`);
	},
	status: {
		alias: "status",
		answer: constant.STATUS_NOTHING_CHANGE
	},
	commit: {
		alias: "commit",
		answer: function () {
			create_commit(`standard_answer/${directory}/commit`);
		}
	},
	log: {
		alias: "log",
		number: 1000,
		answer: create_log(1000),
	}
}
