// 測資五： 一千個檔案，前五百個被 commit 過，後五百個是前五百個的複製品
// 前五百個的檔名分別爲 1, 2, 3, 4, ...., 500, 內容與檔名相同
// 後五百個爲 _1, _2, _3, _4, ...., _500, _1 爲 1 的複製、_2 爲 2 的複製，以此類推

const child_process = require("child_process");
const fs = require("fs");
const constant = require("./constant.js");
const md5 = require('md5');

const directory = "5_many_files"

// 可改變此行來減少檔案數量
const FILE_NUMBER = 500;

function copied() {
	let ret = [];
	for (let i = 1; i <= FILE_NUMBER; i++) {
		ret.push(`${i} => _${i}`);
	}
	return ret.sort().join("\n");
}

function old_md5() {
	let ret = [];
	for (let i = 1; i <= FILE_NUMBER; i++) {
		ret.push(`${i} ${md5(i.toString())}`);
	}
	return ret.sort().join("\n");
}

function all_md5() {
	let ret = [];
	for (let i = 1; i <= FILE_NUMBER; i++) {
		ret.push(`${i} ${md5(i.toString())}`);
		ret.push(`_${i} ${md5(i.toString())}`);
	}
	return ret.sort().join("\n");
}

function old_new_file() {
	let ret = [];
	for (let i = 1; i <= FILE_NUMBER; i++) {
		ret.push(`${i}`);
	}
	return ret.sort().join("\n");
}

const commit_1= `# commit 1
[new_file]
${old_new_file()}
[modified]
[copied]
(MD5)
${old_md5()}
`

const status_content = `[new_file]
[modified]
[copied]
${copied()}
`

const commit_content = `${commit_1}
# commit 2
[new_file]
[modified]
[copied]
${copied()}
(MD5)
${all_md5()}
`;

function create_data() {
	child_process.execSync(`mkdir -p data/${directory}`);
	// 產生一個 90 MiB 的檔案，每個 byte 全爲 0b00000000
	for (let i = 1; i <= FILE_NUMBER; i++) {
		fs.writeFileSync(`data/${directory}/${i}`, i.toString());
		fs.writeFileSync(`data/${directory}/_${i}`, i.toString());
	}

	fs.writeFileSync(`data/${directory}/.loser_record`, commit_1);
}

module.exports = {
	description: "測資五： 一千個檔案，前五百個被 commit 過，後五百個是前五百個的複製品",
	directory: directory,
	create_data: create_data,
	status: {
		alias: "status",
		answer: status_content
	},
	commit: {
		alias: "commit",
		answer: commit_content,
	},
	log: {
		alias: "log",
		number: 0,
		answer: ""
	}
}
