// 測資七： 別名測試

const child_process = require("child_process");
const fs = require("fs");
const constant = require("./constant.js");
const md5 = require("md5");

const directory = "7_alias"

const config_content = `st = status
ci = commit
comm = commit
l = log
`

const status_content = `[new_file]
.loser_config
[modified]
[copied]
`
const commit_content = `# commit 1
[new_file]
.loser_config
[modified]
[copied]
(MD5)
.loser_config ${md5(config_content)}
`;

module.exports = {
	description: "測資七： 別名測試",
	directory: directory,
	create_data: function () {
		child_process.execSync(`mkdir -p data/${directory}`);
		fs.writeFileSync(`data/${directory}/.loser_config`, config_content);
	},
	status: {
		answer: status_content,
		alias: "st",
	},
	commit: {
		answer: commit_content,
		alias: "comm"
	},
	log: {
		number: 1,
		answer: "",
		alias: "l"
	}
}