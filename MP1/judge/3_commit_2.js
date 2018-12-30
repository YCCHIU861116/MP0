// 測資三： commit 了測資二的目錄後，不做任何動作

const child_process = require("child_process");
const fs = require("fs");
const constant = require("./constant.js");

const directory = "3_commit_2"

commit_content = `# commit 1
[new_file]
a
[modified]
[copied]
(MD5)
a 0cc175b9c0f1b6a831c399e269772661
`;

module.exports = {
	description: "測資三： commit 了測資二的目錄後，不做任何動作",
	directory: directory,
	create_data: function () {
		child_process.execSync(`mkdir -p data/${directory}`);
		child_process.execSync(`echo -n "a" > data/${directory}/a`);
		fs.writeFileSync(`data/${directory}/.loser_record`, commit_content);
	},
	status: {
		alias: "status",
		answer: constant.STATUS_NOTHING_CHANGE
	},
	commit: {
		alias: "commit",
		answer: commit_content
	},
	log: {
		alias: "log",
		number: 1,
		answer: commit_content
	}
}
