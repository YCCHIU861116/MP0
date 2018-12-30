// 測資二：目錄下只有一個 a 檔案， a 檔案中只有一個字元 'a'

const child_process = require("child_process")
const constant = require("./constant.js");

const directory = "2_only_a"

module.exports = {
	description: "測資二：目錄下只有一個 a 檔案， a 檔案中只有一個字元 'a'",
	directory: directory,
	create_data: function () {
		child_process.execSync(`mkdir -p data/${directory}`);
		child_process.execSync(`echo -n "a" > data/${directory}/a`)
	},
	status: {
		alias: "status",
		answer: `[new_file]
a
[modified]
[copied]
`
	},
	commit: {
		alias: "commit",
		answer: `# commit 1
[new_file]
a
[modified]
[copied]
(MD5)
a 0cc175b9c0f1b6a831c399e269772661
`
	},
	log: {
		alias: "log",
		number: 1,
		answer: ""
	}
}
