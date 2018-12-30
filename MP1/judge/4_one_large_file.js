// 測資四： 一個大檔案

const child_process = require("child_process");
const fs = require("fs");
const constant = require("./constant.js");

const directory = "4_one_large_file"

const status_content = `[new_file]
large_file
[modified]
[copied]
`

const commit_content = `# commit 1
[new_file]
large_file
[modified]
[copied]
(MD5)
large_file 80284cd454d50ab5a58ff4798bc2e156
`;

module.exports = {
	description: "測資四： 一個大檔案",
	directory: directory,
	create_data: function () {
		child_process.execSync(`mkdir -p data/${directory}`);
		// 產生一個 90 MiB 的檔案，每個 byte 全爲 0b00000000
		child_process.execSync(`dd if=/dev/zero of=data/${directory}/large_file bs=90M count=1`);
	},
	status: {
		alias: "status",
		answer: status_content
	},
	commit: {
		alias: "commit",
		answer: commit_content
	},
	log: {
		alias: "log",
		number: 1,
		answer: ""
	}
}
